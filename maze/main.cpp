#include "pch.h"
#include "states/PacApplication.h"

void show_about_dialog();
bool show_splash_screen(HINSTANCE instance);
void close_splash_screen(ff::window* main_window);

static class pac_host_t : public IPacApplicationHost
{
public:
    virtual void ShowAboutDialog() override
    {
        ff::thread_dispatch::get_main()->post(::show_about_dialog);
    }

    virtual bool IsShowingPopup() const override
    {
        return !::IsWindowEnabled(ff::app_window());
    }

    virtual void Quit() override
    {
        ff::app_window().close();
    }
} pac_host;

static void window_message(ff::window* window, ff::window_message& message)
{
    if (message.msg == WM_SIZE && message.wp == SIZE_MINIMIZED)
    {
        ff::thread_dispatch::get_game()->post([]
        {
            PacApplication::Get()->PauseGame();
        });
    }
}

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    ::show_splash_screen(instance);

    ff::init_game_params params;
    std::unique_ptr<PacApplication> pac_app;

    params.game_clears_back_buffer_func = [] { return true; };
    params.game_update_func = [&] { pac_app->Update(); };
    params.game_render_func = [&](ff::app_update_t, ff::dxgi::command_context_base& context, ff::dxgi::target_base& target) { pac_app->Render(context, target); };
    params.game_thread_initialized_func = [&] { pac_app = std::make_unique<PacApplication>(::pac_host); };
    params.game_thread_finished_func = [&] { pac_app.reset(); };
    params.main_thread_initialized_func = ::close_splash_screen;
    params.main_window_message_func = ::window_message;

    return ff::run_game(params);
}
