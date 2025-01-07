#include "pch.h"
#include "states/PacApplication.h"

void show_about_dialog();
bool show_splash_screen(HINSTANCE instance);
void close_splash_screen(ff::window* main_window);

class pac_host : public ff::game::root_state_base, public IPacApplicationHost
{
public:
    pac_host()
        : pac_app(*this)
    {
        ff::input::pointer().touch_to_mouse(true);
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

    virtual void notify_window_message(ff::window* window, ff::window_message& message) override
    {
        if (message.msg == WM_SIZE && message.wp == SIZE_MINIMIZED)
        {
            ff::thread_dispatch::get_game()->post([&pac_app = this->pac_app]()
            {
                pac_app.PauseGame();
            });
        }
    }

    virtual void ShowAboutDialog() override
    {
        ff::thread_dispatch::get_main()->post(::show_about_dialog);
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

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    ::show_splash_screen(instance);

    ff::game::init_params_t<::pac_host> params;
    params.window_initialized_func = ::close_splash_screen;

    return ff::game::run(params);
}
