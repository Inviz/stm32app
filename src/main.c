#ifdef APP_MOTHERSHIP
    #include "app/mothership.h"
#endif

extern void initialise_monitor_handles(void);

static void app_boot(app_t **app) {
#if APP_MOTHERSHIP
    log_printf("App - Mothership ...\n");
    log_printf("App - Enumerating devices ...\n");
    app_allocate(app, OD, app_mothership_enumerate_devices);
#endif
    log_printf("App - Constructing...\n");
    app_set_phase(*app, DEVICE_CONSTRUCTING);

    log_printf("App - Linking...\n");
    app_set_phase(*app, DEVICE_LINKING);

    log_printf("App - Starting...\n");
    app_set_phase(*app, DEVICE_STARTING);
}



int main(void) {
#ifdef SEMIHOSTING
    initialise_monitor_handles();
#endif
    app_t *app;
    app_boot(&app); 
    log_printf("App - Starting tasks...\n");
    vTaskStartScheduler();
    while (true) { }
    return 0;
}
