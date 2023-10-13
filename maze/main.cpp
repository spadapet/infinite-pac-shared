#include "pch.h"
#include "splash_screen.h"
#include "states/PacApplication.h"

class root_state : public ff::game::root_state_base, public IPacApplicationHost
{
public:
    root_state()
        : pac_app(*this)
    {}

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
    }

    virtual bool IsShowingPopup() const override
    {
        return false;
    }

    virtual void SetPaused(bool value) override
    {
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

    ff::game::init_params_t<::root_state> params;
    params.window_initialized_func = std::bind(::close_splash_screen);

    return ff::game::run(params);
}
