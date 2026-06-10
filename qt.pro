QT += core gui network websockets webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt
TEMPLATE = app

# Keep generated build artifacts out of the project root.
BUILD_ARTIFACTS_DIR = $$OUT_PWD/.build
OBJECTS_DIR = $$BUILD_ARTIFACTS_DIR/obj
MOC_DIR = $$BUILD_ARTIFACTS_DIR/moc
RCC_DIR = $$BUILD_ARTIFACTS_DIR/rcc
UI_DIR = $$BUILD_ARTIFACTS_DIR/ui

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        src/app_controller.cpp \
        src/coordinators/home_coordinator.cpp \
        src/coordinators/settings_coordinator.cpp \
        src/main.cpp \
        src/mainwindow.cpp \
        src/network/core/tcp_manager.cpp \
        src/network/dispatch/tcp_manager_base_reports.cpp \
        src/network/dispatch/tcp_manager_device_delegates.cpp \
        src/network/dispatch/tcp_manager_settings_delegates.cpp \
        src/network/dispatch/tcp_manager_spectrum_delegates.cpp \
        src/components/screen_flash_overlay.cpp \
        src/components/top_nav_bar.cpp \
        src/services/calibration_service.cpp \
        src/services/drone_ops_service.cpp \
        src/services/device_ops_service.cpp \
        src/services/spectrum_service.cpp \
        src/services/settings_protocol_service.cpp \
        src/services/local_time_service_client.cpp \
        src/views/history/history_page.cpp \
        src/views/home/home_page.cpp \
        src/views/home/home_web_bridge.cpp \
        src/views/home/bottom_console.cpp \
        src/views/statistics/stats_page.cpp \
        src/views/settings/alarm_history/alarm_history_page.cpp \
        src/views/settings/angle_calibration/angle_calibration_page.cpp \
        src/views/settings/authorization_info/authorization_info_page.cpp \
        src/views/settings/data_collection/data_collection_page.cpp \
        src/views/settings/direction_calibration_value/direction_calibration_value_page.cpp \
        src/views/settings/detect_band/detect_band_page.cpp \
        src/views/settings/device/device_settings_page.cpp \
        src/views/settings/firmware_version/firmware_version_page.cpp \
        src/views/settings/model_library/model_library_edit_dialog.cpp \
        src/views/settings/model_library/model_library_page.cpp \
        src/views/settings/mode_select/mode_select_page.cpp \
        src/views/settings/power_amplifier/power_amplifier_page.cpp \
        src/views/settings/signal_source_params/signal_source_params_page.cpp \
        src/views/settings/strike_frequency/strike_frequency_page.cpp \
        src/views/settings/strike_status/strike_status_page.cpp \
        src/views/settings/spectrum_switch/spectrum_switch_page.cpp \
        src/views/settings/system_log/system_log_page.cpp \
        src/views/settings/system_function/system_function_page.cpp \
        src/views/settings/settings_page.cpp \
        src/views/settings/device/map_picker_dialog.cpp \
        src/views/whitelist/whitelist_page.cpp

HEADERS += \
        src/app_config.h \
        src/app_controller.h \
        src/coordinators/home_coordinator.h \
        src/coordinators/settings_coordinator.h \
        src/device_status.h \
        src/mainwindow.h \
        src/network/core/protocol_types.h \
        src/network/core/tcp_manager.h \
        src/components/screen_flash_overlay.h \
        src/components/top_nav_bar.h \
        src/services/calibration_service.h \
        src/services/drone_ops_service.h \
        src/services/device_ops_service.h \
        src/services/spectrum_service.h \
        src/services/settings_protocol_service.h \
        src/services/local_time_service_client.h \
        src/views/history/history_page.h \
        src/views/home/home_page.h \
        src/views/home/home_web_bridge.h \
        src/views/home/bottom_console.h \
        src/views/statistics/stats_page.h \
        src/views/settings/alarm_history/alarm_history_page.h \
        src/views/settings/angle_calibration/angle_calibration_page.h \
        src/views/settings/authorization_info/authorization_info_page.h \
        src/views/settings/data_collection/data_collection_page.h \
        src/views/settings/direction_calibration_value/direction_calibration_value_page.h \
        src/views/settings/detect_band/detect_band_page.h \
        src/views/settings/device/device_settings_page.h \
        src/views/settings/firmware_version/firmware_version_page.h \
        src/views/settings/model_library/model_library_edit_dialog.h \
        src/views/settings/model_library/model_library_page.h \
        src/views/settings/mode_select/mode_select_page.h \
        src/views/settings/power_amplifier/power_amplifier_page.h \
        src/views/settings/signal_source_params/signal_source_params_page.h \
        src/views/settings/strike_frequency/strike_frequency_page.h \
        src/views/settings/strike_status/strike_status_page.h \
        src/views/settings/spectrum_switch/spectrum_switch_page.h \
        src/views/settings/system_log/system_log_page.h \
        src/views/settings/system_function/system_function_page.h \
        src/views/settings/settings_page.h \
        src/views/settings/settings_role.h \
        src/views/settings/device/map_picker_dialog.h \
        src/views/whitelist/whitelist_page.h

INCLUDEPATH += \
        $$PWD/src \
        $$PWD/src/coordinators \
        $$PWD/src/components \
        $$PWD/src/network/core \
        $$PWD/src/network/dispatch \
        $$PWD/src/services \
        $$PWD/src/views \
        $$PWD/src/views/home \
        $$PWD/src/views/history \
        $$PWD/src/views/settings \
        $$PWD/src/views/settings/alarm_history \
        $$PWD/src/views/settings/angle_calibration \
        $$PWD/src/views/settings/authorization_info \
        $$PWD/src/views/settings/data_collection \
        $$PWD/src/views/settings/direction_calibration_value \
        $$PWD/src/views/settings/detect_band \
        $$PWD/src/views/settings/device \
        $$PWD/src/views/settings/firmware_version \
        $$PWD/src/views/settings/model_library \
        $$PWD/src/views/settings/mode_select \
        $$PWD/src/views/settings/power_amplifier \
        $$PWD/src/views/settings/signal_source_params \
        $$PWD/src/views/settings/strike_frequency \
        $$PWD/src/views/settings/strike_status \
        $$PWD/src/views/settings/spectrum_switch \
        $$PWD/src/views/settings/system_log \
        $$PWD/src/views/settings/system_function \
        $$PWD/src/views/statistics \
        $$PWD/src/views/whitelist
