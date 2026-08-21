#include "alarm_preferences.h"

#include <QSettings>

namespace
{
const char kSettingsOrganization[] = "JunGongDun";
const char kSettingsApplication[] = "QtClient";
const char kScreenFlashEnabledKey[] = "alarm/screenFlashEnabled";
} // namespace

namespace AlarmPreferences
{
bool screenFlashEnabled()
{
    QSettings settings(kSettingsOrganization, kSettingsApplication);
    return settings.value(QLatin1String(kScreenFlashEnabledKey), false).toBool();
}

void setScreenFlashEnabled(bool enabled)
{
    QSettings settings(kSettingsOrganization, kSettingsApplication);
    settings.setValue(QLatin1String(kScreenFlashEnabledKey), enabled);
}
} // namespace AlarmPreferences
