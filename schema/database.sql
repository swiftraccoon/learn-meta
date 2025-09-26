-- PostgreSQL schema for SDRTrunk Radio API
-- Complete schema with ALL RdioScanner/SDRTrunk fields

-- Drop existing tables if they exist
DROP TABLE IF EXISTS radio_calls CASCADE;
DROP TABLE IF EXISTS system_stats CASCADE;

-- Main radio calls table with ALL fields from SDRTrunk rdio-api
CREATE TABLE radio_calls (
    -- Primary key
    id SERIAL PRIMARY KEY,

    -- Required core fields
    api_key VARCHAR(100) NOT NULL,
    system_id VARCHAR(10) NOT NULL,
    timestamp TIMESTAMPTZ NOT NULL,

    -- Audio file information
    audio_filename VARCHAR(500),
    audio_content_type VARCHAR(100),
    audio_size INTEGER,
    audio_path VARCHAR(500),  -- Where we store the file locally

    -- Radio metadata
    frequency BIGINT,
    talkgroup INTEGER,
    source INTEGER,  -- Source radio ID

    -- Labels and descriptions
    system_label VARCHAR(255),
    talkgroup_label VARCHAR(255),
    talkgroup_group VARCHAR(255),
    talkgroup_tag VARCHAR(255),
    talker_alias VARCHAR(255),

    -- Multiple values (stored as comma-separated or JSON)
    patches TEXT,  -- Comma-separated list of patched talkgroups
    frequencies TEXT,  -- Comma-separated list of frequencies
    sources TEXT,  -- Comma-separated list of source IDs

    -- Upload metadata
    upload_ip INET,
    upload_time TIMESTAMPTZ DEFAULT NOW(),
    test_mode BOOLEAN DEFAULT FALSE,

    -- Processing status
    processed BOOLEAN DEFAULT FALSE,
    processing_time TIMESTAMPTZ,
    error_message TEXT,

    -- Additional metadata as JSON for flexibility
    metadata JSONB,

    -- Timestamps
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Indexes for performance
CREATE INDEX idx_radio_calls_timestamp ON radio_calls(timestamp);
CREATE INDEX idx_radio_calls_system_id ON radio_calls(system_id);
CREATE INDEX idx_radio_calls_talkgroup ON radio_calls(talkgroup);
CREATE INDEX idx_radio_calls_frequency ON radio_calls(frequency);
CREATE INDEX idx_radio_calls_source ON radio_calls(source);
CREATE INDEX idx_radio_calls_created_at ON radio_calls(created_at);
CREATE INDEX idx_radio_calls_processed ON radio_calls(processed);
CREATE INDEX idx_radio_calls_test_mode ON radio_calls(test_mode);
CREATE INDEX idx_radio_calls_api_key ON radio_calls(api_key);

-- GIN index for JSONB metadata searches
CREATE INDEX idx_radio_calls_metadata ON radio_calls USING GIN (metadata);

-- System statistics table
CREATE TABLE system_stats (
    id SERIAL PRIMARY KEY,
    system_id VARCHAR(10) NOT NULL UNIQUE,
    system_label VARCHAR(255),
    total_calls BIGINT DEFAULT 0,
    calls_today INTEGER DEFAULT 0,
    calls_this_hour INTEGER DEFAULT 0,
    last_call_time TIMESTAMPTZ,
    first_seen TIMESTAMPTZ DEFAULT NOW(),
    last_updated TIMESTAMPTZ DEFAULT NOW()
);

-- Talkgroup statistics table
CREATE TABLE talkgroup_stats (
    id SERIAL PRIMARY KEY,
    system_id VARCHAR(10) NOT NULL,
    talkgroup INTEGER NOT NULL,
    talkgroup_label VARCHAR(255),
    talkgroup_group VARCHAR(255),
    total_calls BIGINT DEFAULT 0,
    last_call_time TIMESTAMPTZ,
    first_seen TIMESTAMPTZ DEFAULT NOW(),
    last_updated TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE(system_id, talkgroup)
);

-- Source (radio) statistics table
CREATE TABLE source_stats (
    id SERIAL PRIMARY KEY,
    system_id VARCHAR(10) NOT NULL,
    source INTEGER NOT NULL,
    talker_alias VARCHAR(255),
    total_calls BIGINT DEFAULT 0,
    last_call_time TIMESTAMPTZ,
    first_seen TIMESTAMPTZ DEFAULT NOW(),
    last_updated TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE(system_id, source)
);

-- API key management table
CREATE TABLE api_keys (
    id SERIAL PRIMARY KEY,
    key VARCHAR(100) NOT NULL UNIQUE,
    name VARCHAR(255),
    description TEXT,
    active BOOLEAN DEFAULT TRUE,
    rate_limit INTEGER DEFAULT 100,  -- calls per minute
    total_calls BIGINT DEFAULT 0,
    last_call_time TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Upload statistics by IP
CREATE TABLE upload_stats (
    id SERIAL PRIMARY KEY,
    upload_ip INET NOT NULL UNIQUE,
    total_uploads BIGINT DEFAULT 0,
    last_upload_time TIMESTAMPTZ,
    first_seen TIMESTAMPTZ DEFAULT NOW(),
    blocked BOOLEAN DEFAULT FALSE,
    block_reason TEXT
);

-- Function to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Triggers for updated_at
CREATE TRIGGER update_radio_calls_updated_at BEFORE UPDATE
    ON radio_calls FOR EACH ROW EXECUTE PROCEDURE
    update_updated_at_column();

CREATE TRIGGER update_system_stats_updated_at BEFORE UPDATE
    ON system_stats FOR EACH ROW EXECUTE PROCEDURE
    update_updated_at_column();

CREATE TRIGGER update_talkgroup_stats_updated_at BEFORE UPDATE
    ON talkgroup_stats FOR EACH ROW EXECUTE PROCEDURE
    update_updated_at_column();

CREATE TRIGGER update_source_stats_updated_at BEFORE UPDATE
    ON source_stats FOR EACH ROW EXECUTE PROCEDURE
    update_updated_at_column();

CREATE TRIGGER update_api_keys_updated_at BEFORE UPDATE
    ON api_keys FOR EACH ROW EXECUTE PROCEDURE
    update_updated_at_column();

-- Function to update statistics after insert
CREATE OR REPLACE FUNCTION update_call_statistics()
RETURNS TRIGGER AS $$
BEGIN
    -- Update system stats
    INSERT INTO system_stats (system_id, system_label, total_calls, calls_today, calls_this_hour, last_call_time)
    VALUES (NEW.system_id, NEW.system_label, 1, 1, 1, NEW.timestamp)
    ON CONFLICT (system_id) DO UPDATE SET
        total_calls = system_stats.total_calls + 1,
        calls_today = CASE
            WHEN DATE(system_stats.last_call_time) = DATE(NEW.timestamp)
            THEN system_stats.calls_today + 1
            ELSE 1
        END,
        calls_this_hour = CASE
            WHEN system_stats.last_call_time > NOW() - INTERVAL '1 hour'
            THEN system_stats.calls_this_hour + 1
            ELSE 1
        END,
        last_call_time = NEW.timestamp,
        system_label = COALESCE(NEW.system_label, system_stats.system_label);

    -- Update talkgroup stats if talkgroup is present
    IF NEW.talkgroup IS NOT NULL THEN
        INSERT INTO talkgroup_stats (system_id, talkgroup, talkgroup_label, talkgroup_group, total_calls, last_call_time)
        VALUES (NEW.system_id, NEW.talkgroup, NEW.talkgroup_label, NEW.talkgroup_group, 1, NEW.timestamp)
        ON CONFLICT (system_id, talkgroup) DO UPDATE SET
            total_calls = talkgroup_stats.total_calls + 1,
            last_call_time = NEW.timestamp,
            talkgroup_label = COALESCE(NEW.talkgroup_label, talkgroup_stats.talkgroup_label),
            talkgroup_group = COALESCE(NEW.talkgroup_group, talkgroup_stats.talkgroup_group);
    END IF;

    -- Update source stats if source is present
    IF NEW.source IS NOT NULL THEN
        INSERT INTO source_stats (system_id, source, talker_alias, total_calls, last_call_time)
        VALUES (NEW.system_id, NEW.source, NEW.talker_alias, 1, NEW.timestamp)
        ON CONFLICT (system_id, source) DO UPDATE SET
            total_calls = source_stats.total_calls + 1,
            last_call_time = NEW.timestamp,
            talker_alias = COALESCE(NEW.talker_alias, source_stats.talker_alias);
    END IF;

    -- Update API key stats
    UPDATE api_keys SET
        total_calls = total_calls + 1,
        last_call_time = NEW.timestamp
    WHERE key = NEW.api_key;

    -- Update upload stats
    IF NEW.upload_ip IS NOT NULL THEN
        INSERT INTO upload_stats (upload_ip, total_uploads, last_upload_time)
        VALUES (NEW.upload_ip, 1, NEW.upload_time)
        ON CONFLICT (upload_ip) DO UPDATE SET
            total_uploads = upload_stats.total_uploads + 1,
            last_upload_time = NEW.upload_time;
    END IF;

    RETURN NEW;
END;
$$ language 'plpgsql';

-- Trigger to update statistics on new call insert
CREATE TRIGGER update_statistics_on_insert
    AFTER INSERT ON radio_calls
    FOR EACH ROW EXECUTE PROCEDURE update_call_statistics();

-- Insert a default API key for testing
INSERT INTO api_keys (key, name, description)
VALUES ('test-api-key-12345', 'Test Key', 'Default test API key for development')
ON CONFLICT DO NOTHING;