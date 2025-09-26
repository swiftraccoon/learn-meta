// radio.thrift - Thrift IDL for SDRTrunk RdioScanner API

namespace cpp radio
namespace py radio
namespace rs radio_api
namespace go radio

struct RadioCall {
  // Required core fields
  1: required string apiKey          // API key for authentication
  2: required string systemId        // System ID (numeric string)
  3: required i64 timestamp          // Unix timestamp in seconds

  // Audio file information
  4: optional string audioFilename   // Uploaded audio filename
  5: optional string audioContentType // MIME type of audio file
  6: optional i32 audioSize          // Size of audio file in bytes
  7: optional string audioPath       // Local storage path

  // Radio metadata
  8: optional i64 frequency          // Frequency in Hz
  9: optional i32 talkgroup          // Talkgroup ID
  10: optional i32 source            // Source radio ID

  // Labels and descriptions
  11: optional string systemLabel    // Human-readable system name
  12: optional string talkgroupLabel // Human-readable talkgroup name
  13: optional string talkgroupGroup // Talkgroup category/group
  14: optional string talkgroupTag   // Additional talkgroup tag
  15: optional string talkerAlias    // Alias of the talking radio

  // Multiple values (comma-separated strings)
  16: optional string patches        // Comma-separated list of patched talkgroups
  17: optional string frequencies    // Comma-separated list of frequencies
  18: optional string sources        // Comma-separated list of source IDs

  // Upload metadata
  19: optional string uploadIp       // IP address of uploader
  20: optional i64 uploadTime        // Upload timestamp
  21: optional bool testMode         // Test mode flag

  // Processing status
  22: optional bool processed        // Has this call been processed
  23: optional i64 processingTime    // When it was processed
  24: optional string errorMessage   // Any error during processing

  // Additional metadata
  25: optional map<string, string> metadata  // Flexible metadata storage
}

// Call statistics
struct CallStats {
  1: i64 totalCalls                  // Total number of calls
  2: i32 callsToday                  // Calls received today
  3: i32 callsThisHour              // Calls in the last hour
  4: map<string, i64> callsPerSystem // Breakdown by system
  5: map<string, i64> callsPerTalkgroup // Breakdown by talkgroup
}

// System information
struct SystemInfo {
  1: string systemId
  2: optional string systemLabel
  3: i64 totalCalls
  4: optional i64 lastCallTime
  5: optional i64 firstSeen
}

// Talkgroup information
struct TalkgroupInfo {
  1: string systemId
  2: i32 talkgroup
  3: optional string talkgroupLabel
  4: optional string talkgroupGroup
  5: i64 totalCalls
  6: optional i64 lastCallTime
}

// Source/Radio information
struct SourceInfo {
  1: string systemId
  2: i32 source
  3: optional string talkerAlias
  4: i64 totalCalls
  5: optional i64 lastCallTime
}

// Upload response
struct UploadResponse {
  1: string status                   // "ok" or "error"
  2: string message                  // Human-readable message
  3: optional string callId          // Unique identifier for the call
}

// Health check response
struct HealthStatus {
  1: string status                   // "healthy", "degraded", or "unhealthy"
  2: i64 timestamp                   // Current server time
  3: string version                  // API version
  4: string databaseStatus           // Database connection status
  5: optional map<string, string> details // Additional health details
}

// API Key information
struct ApiKeyInfo {
  1: string key
  2: optional string name
  3: optional string description
  4: bool active
  5: i32 rateLimit                  // Calls per minute
  6: i64 totalCalls
  7: optional i64 lastCallTime
}

// Query parameters for listing calls
struct CallQuery {
  1: optional string systemId
  2: optional i32 talkgroup
  3: optional i32 source
  4: optional i64 startTime
  5: optional i64 endTime
  6: optional i32 limit
  7: optional i32 offset
  8: optional bool includeTest      // Include test mode calls
}

// Service definition with all methods
service RadioService {
  // Core upload endpoint (matches RdioScanner API)
  UploadResponse uploadCall(1: RadioCall call)

  // Query endpoints
  list<RadioCall> getRecentCalls(1: i32 limit)
  list<RadioCall> queryCalls(1: CallQuery query)
  RadioCall getCall(1: string callId)

  // Statistics
  CallStats getStatistics()
  list<SystemInfo> getSystems()
  list<TalkgroupInfo> getTalkgroups(1: string systemId)
  list<SourceInfo> getSources(1: string systemId)

  // Health and status
  HealthStatus getHealth()

  // Admin functions
  ApiKeyInfo createApiKey(1: string name, 2: string description)
  bool validateApiKey(1: string key)
  bool revokeApiKey(1: string key)
  list<ApiKeyInfo> listApiKeys()

  // Audio file management
  binary getAudioFile(1: string callId)
  bool deleteAudioFile(1: string callId)

  // Bulk operations
  i32 deleteOldCalls(1: i64 beforeTimestamp)
  i32 processUnprocessedCalls()
}