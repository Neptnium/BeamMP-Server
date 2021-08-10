#include "TSentry.h"
#include "Common.h"

TSentry::TSentry(const std::string& SentryUrl) {
    if (SentryUrl.empty()) {
        mValid = false;
    } else {
        mValid = true;
        sentry_options_t* options = sentry_options_new();
        sentry_options_set_dsn(options, SentryUrl.c_str());
        auto ReleaseString = "BeamMP-Server@" + Application::ServerVersion();
        sentry_options_set_release(options, ReleaseString.c_str());
        sentry_options_set_max_breadcrumbs(options, 10);
        sentry_init(options);
    }
}

TSentry::~TSentry() {
    if (mValid) {
        sentry_close();
    }
}

void TSentry::PrintWelcome() {
    if (mValid) {
        info("Sentry started");
    } else {
        info("Sentry disabled in unofficial build");
    }
}

void TSentry::SetupUser() {
    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(user, "id", sentry_value_new_string(Application::Settings.Key.c_str()));
    sentry_value_set_by_key(user, "authkey", sentry_value_new_string(Application::Settings.Key.c_str()));
    sentry_set_user(user);
}

void TSentry::Log(sentry_level_t level, const std::string& logger, const std::string& text) {
    if (!mValid) {
        return;
    }
    auto Msg = sentry_value_new_message_event(level, logger.c_str(), text.c_str());
    sentry_capture_event(Msg);
    sentry_remove_transaction();
}

void TSentry::LogDebug(const std::string& text, const std::string& file, const std::string& line) {
    SetTransaction(file + ":" + line);
    Log(SENTRY_LEVEL_DEBUG, "default", file + ": " + text);
}

void TSentry::AddExtra(const std::string& key, const sentry_value_t& value) {
    if (!mValid) {
        return;
    }
    sentry_set_extra(key.c_str(), value);
}

void TSentry::AddExtra(const std::string& key, const std::string& value) {
    if (!mValid) {
        return;
    }
    AddExtra(key.c_str(), sentry_value_new_string(value.c_str()));
}

void TSentry::LogException(const std::exception& e, const std::string& file, const std::string& line) {
    if (!mValid) {
        return;
    }
    Log(SENTRY_LEVEL_ERROR, "exceptions", std::string(e.what()) + " @ " + file + ":" + line);
}

void TSentry::AddErrorBreadcrumb(const std::string& msg, const std::string& file, const std::string& line) {
    if (!mValid) {
        return;
    }
    auto crumb = sentry_value_new_breadcrumb("default", (msg + " @ " + file + ":" + line).c_str());
    sentry_value_set_by_key(crumb, "level", sentry_value_new_string("error"));
    sentry_add_breadcrumb(crumb);
}

void TSentry::SetTransaction(const std::string& id) {
    if (!mValid) {
        return;
    }
    sentry_set_transaction(id.c_str());
}

std::unique_lock<std::mutex> TSentry::CreateExclusiveContext() {
    return std::unique_lock<std::mutex>(mMutex);
}