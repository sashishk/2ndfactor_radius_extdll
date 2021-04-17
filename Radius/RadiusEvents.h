//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: CATID_GENERAL
//
// MessageText:
//
//  General
//
#define CATID_GENERAL                    ((WORD)0x20000001L)

//
// MessageId: EVMSG_STARTUP
//
// MessageText:
//
//  The PeoplePC Radius extensions have been loaded.
//
#define EVMSG_STARTUP                    ((DWORD)0x60000100L)

//
// MessageId: EVMSG_SHUTDOWN
//
// MessageText:
//
//  The PeoplePC Radius extensions have been unloaded.
//
#define EVMSG_SHUTDOWN                   ((DWORD)0x60000101L)

//
// MessageId: EVMSG_HITLOG
//
// MessageText:
//
//  RadiusDB Log: %1
//
#define EVMSG_HITLOG                     ((DWORD)0x60000102L)

//
// MessageId: EVMSG_BADCONFIG
//
// MessageText:
//
//  Missing configuration setting! (%1)
//
#define EVMSG_BADCONFIG                  ((DWORD)0xE0000103L)

//
// MessageId: EVMSG_XMLBAD
//
// MessageText:
//
//  XML configuration invalid: %1 Source Text: %2
//
#define EVMSG_XMLBAD                     ((DWORD)0xE0000104L)

//
// MessageId: EVMSG_COMERROR
//
// MessageText:
//
//  COM error caught. Source: %1 Description: %2 Error Message: %3
//
#define EVMSG_COMERROR                   ((DWORD)0xE0000105L)

//
// MessageId: EVMSG_ACCEPTED
//
// MessageText:
//
//  User auth request accepted.
//
#define EVMSG_ACCEPTED                   ((DWORD)0x60000106L)

//
// MessageId: EVMSG_REJECTED
//
// MessageText:
//
//  User auth request rejected.
//
#define EVMSG_REJECTED                   ((DWORD)0x60000107L)

//
// MessageId: EVMSG_EXCEPTION
//
// MessageText:
//
//  Caught unknown exception during request processing.
//
#define EVMSG_EXCEPTION                  ((DWORD)0xE0000108L)

//
// MessageId: EVMSG_TLSINIT
//
// MessageText:
//
//  Thread local DB Interface initializing.
//
#define EVMSG_TLSINIT                    ((DWORD)0x60000109L)

//
// MessageId: EVMSG_ACCT
//
// MessageText:
//
//  Received accounting packet.
//
#define EVMSG_ACCT                       ((DWORD)0x6000010AL)

//
// MessageId: EVMSG_ATTRETURNFAIL
//
// MessageText:
//
//  Failure translating outgoing attributes!  Check return of LoginRequest stored procedure.
//
#define EVMSG_ATTRETURNFAIL              ((DWORD)0xE000010AL)

