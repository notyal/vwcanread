// Configuration for project
#ifndef CONFIG_H
#define CONFIG_H

// *****************************************************************************
// GENERAL OPTIONS
// *****************************************************************************
#define SERIAL_BUF_SIZE 256

// *****************************************************************************
// CAN LIBRARY OPTIONS
// *****************************************************************************

#define CANLIB_SEEED
// #define CANLIB_MCNEIGHT

// *****************************************************************************
// DEBUG OPTIONS
// *****************************************************************************

// #define DEBUG_MEMORY

// *****************************************************************************
// Sanity checks
// *****************************************************************************

#if defined(CANLIB_SEEED) && defined(CANLIB_MCNEIGHT)
#error "Cannot define more than one library."
#endif

#endif /* CONFIG_H */
