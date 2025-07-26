#ifndef __UTILS_SETUP_H__
#define __UTILS_SETUP_H__

/**
 * Sets up some expected state for the app. It should run before anything else
 * has a chance to.
 * @author Aryan Jassal
 */
void setup();

/**
 * Cleans up the resources setup by bootstrapping. This should run after all the
 * logic right before exiting the application. If the application is exiting
 * ungracefully, then this isn't required.
 * @author Aryan Jassal
 */
void teardown();

#endif