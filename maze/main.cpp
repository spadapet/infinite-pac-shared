#include "pch.h"
#include "splash_screen.h"
#include "states/PacApplication.h"

class root_state : public ff::game::root_state_base, public IPacApplicationHost
{
public:
    root_state()
        : pac_app(*this)
    {
    }

    virtual std::shared_ptr<ff::state> advance_time() override
    {
        this->pac_app.Advance();
        return {};
    }

    virtual void render(ff::dxgi::command_context_base& context, ff::render_targets& targets) override
    {
        this->pac_app.Render(context, targets);
    }

    virtual void ShowAboutDialog() override
    {
        ff::thread_dispatch::get_main()->post([]()
        {
            ::MessageBox(ff::app_window(), L"Test about dialog", L"About", MB_OK);
        });
    }

    virtual bool IsShowingPopup() const override
    {
        return !::IsWindowEnabled(ff::app_window());
    }

protected:
    virtual void save_settings(ff::dict& dict)
    {
        this->pac_app.SaveState();
    }

    virtual void load_settings(const ff::dict& dict)
    {
        this->pac_app.LoadState();
    }

private:
    PacApplication pac_app;
};

ff::signal_connection window_connection;

static void window_message(ff::window* window, ff::window_message& message)
{
    switch (message.msg)
    {
        case WM_SIZE:
            if (message.wp == SIZE_MINIMIZED)
            {
                ff::thread_dispatch::get_game()->post([]()
                {
                    if (PacApplication::Get())
                    {
                        PacApplication::Get()->PauseGame();
                    }
                });
            }
            break;

        case WM_DESTROY:
            ::window_connection.disconnect();
            break;
    }
}

static void window_initialized(ff::window* window)
{
    ::close_splash_screen();

    ::window_connection = window->message_sink().connect(::window_message);
}

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    ::show_splash_screen(instance);

    ff::game::init_params_t<::root_state> params;
    params.window_initialized_func = ::window_initialized;

    return ff::game::run(params);
}
