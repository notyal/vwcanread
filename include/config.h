// Configuration for project
#ifndef CONFIG_H
#define CONFIG_H

// *****************************************************************************
// DEBUG OPTIONS
// *****************************************************************************

// #define DEBUG_MEMORY

// *****************************************************************************
// CAN LIBRARY OPTIONS
// *****************************************************************************

#define CANLIB_SEEED
// #define CANLIB_MCNEIGHT

// *****************************************************************************
// Sanity checks
// *****************************************************************************

#if defined(CANLIB_SEEED) && defined(CANLIB_MCNEIGHT)
#error "Cannot define more than one library."
#endif

#endif /* CONFIG_H */
