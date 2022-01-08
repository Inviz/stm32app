#ifdef APP_MOTHERSHIP
    #include "app/mothership.h"
#endif

extern void initialise_monitor_handles(void);

// Global variable is only used in interrupts
app_t *app;

/* main ***********************************************************************/
int main(void) {
#ifdef SEMIHOSTING
    initialise_monitor_handles(); /* This Function MUST come before the first log_printf() */
#endif
    log_printf("App - Enumerating...\n");
#if APP_MOTHERSHIP
    app_allocate(&app, OD, app_mothership_enumerate_devices);
#endif
    log_printf("App - Constructing...\n");
    app_set_phase(app, DEVICE_CONSTRUCTING);

    log_printf("App - Linking...\n");
    app_set_phase(app, DEVICE_LINKING);

    log_printf("App - Starting...\n");
    app_set_phase(app, DEVICE_STARTING);

    log_printf("App - Starting tasks...\n");
    vTaskStartScheduler();
    for (;;)
        ;
    return 0;
}
