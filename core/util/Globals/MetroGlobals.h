#pragma once

#include "Dict/Dict.h"
#include "Globals/AppGlobals.h"
#include "Types/Timer.h"
#include "Windows/Handles.h"

#if METRO_APP

ref class MetroGlobalsEvents;

namespace ff
{
	class IDataWriter;
	class IRenderTargetWindow;
	class IThreadDispatch;
	class ProcessGlobals;
	class State;

	enum class MetroGlobalsFlags
	{
		None = 0,
		UseGraphics = 1 << 0,
		UseAudio = 1 << 1,
		UsePointer = 1 << 2,
		UseKeyboard = 1 << 3,
		UseJoystick = 1 << 4,
		UseRender2d = 1 << 5,
		UseGameLoop = 1 << 6,

		All =
			UseGraphics |
			UseAudio |
			UsePointer |
			UseKeyboard |
			UseJoystick |
			UseRender2d |
			UseGameLoop
	};

	class MetroGlobals : public AppGlobals
	{
	public:
		UTIL_API MetroGlobals();
		UTIL_API virtual ~MetroGlobals();

		UTIL_API static MetroGlobals *Get();

		typedef std::function<bool(MetroGlobals *globals)> FinishInitFunc;
		typedef std::function<std::shared_ptr<ff::State>(MetroGlobals *globals)> InitialStateFactory;
		UTIL_API bool Startup(
			MetroGlobalsFlags flags,
			Windows::UI::Xaml::Window ^window,
			Windows::UI::Xaml::Controls::SwapChainPanel ^swapPanel,
			Windows::Graphics::Display::DisplayInformation ^displayInfo,
			FinishInitFunc finishInitFunc,
			InitialStateFactory initialStateFactory);
		UTIL_API void Shutdown();

		// AppGlobals
		UTIL_API virtual IRenderTargetWindow *GetTarget() const override;
		UTIL_API virtual IRenderDepth *GetDepth() const override;
		UTIL_API virtual IGraphDevice *GetGraph() const override;
		UTIL_API virtual IAudioDevice *GetAudio() const override;
		UTIL_API virtual IPointerDevice *GetPointer() const override;
		UTIL_API virtual IKeyboardDevice *GetKeys() const override;
		UTIL_API virtual IJoystickInput *GetJoysticks() const override;
		UTIL_API virtual I2dRenderer *Get2dRender() const override;
		UTIL_API virtual I2dEffect *Get2dEffect() const override;
		UTIL_API virtual IThreadDispatch *GetGameDispatch() const override;
		UTIL_API virtual const GlobalTime &GetGlobalTime() const override;
		UTIL_API virtual const FrameTime &GetFrameTime() const override;
		UTIL_API virtual void PostGameEvent(hash_t eventId, int data) override;
		UTIL_API virtual bool SendGameEvent(hash_t eventId, int data1 = 0, void *data2 = nullptr) override;

		UTIL_API String GetLogFile() const;
		UTIL_API Windows::UI::Xaml::Window ^GetWindow() const;
		UTIL_API Windows::UI::Xaml::Controls::SwapChainPanel ^GetSwapChainPanel() const;
		UTIL_API Windows::Graphics::Display::DisplayInformation ^GetDisplayInfo() const;

		UTIL_API double GetDpiScale() const;
		UTIL_API double PixelToDip(double size) const;
		UTIL_API double DipToPixel(double size) const;

		// App state
		UTIL_API bool LoadState();
		UTIL_API bool SaveState();
		UTIL_API Dict GetState();
		UTIL_API void SetState(const Dict &dict);
		UTIL_API void ClearAllState();
		UTIL_API String GetStateFile() const;
		UTIL_API virtual Dict GetState(StringRef name) override;
		UTIL_API virtual void SetState(StringRef name, const Dict &dict) override;

	private:
		friend ref class ::MetroGlobalsEvents;

		// Event handlers
		void OnAppSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^args);
		void OnAppResuming(Platform::Object ^sender, Platform::Object ^arg);
		void OnWindowActivated(Platform::Object ^sender, Windows::UI::Core::WindowActivatedEventArgs ^args);
		void OnVisibilityChanged(Platform::Object ^sender, Windows::UI::Core::VisibilityChangedEventArgs ^args);
		void OnWindowClosed(Platform::Object ^sender, Windows::UI::Core::CoreWindowEventArgs ^args);
		void OnSizeChanged(Platform::Object ^sender, Windows::UI::Core::WindowSizeChangedEventArgs ^args);
		void OnLogicalDpiChanged(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender);
		void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^panel, Platform::Object ^sender);
		void OnSwapChainSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^args);

		// Update/render loop
		void StartRenderLoop();
		void StopRenderLoop();
		void RunRenderLoop(bool run);
		void RenderLoopThread();
		void UpdateSwapChain();
		void UpdateDpiScale();
		void ValidateGraphDevice();

		// Update during game loop
		bool FrameAdvanceAndRender();
		void FrameAdvanceResources();
		bool FrameRender();
		void FrameResetTimer();
		void FrameStartTimer();
		bool FrameAdvanceTimer();

		// Initialization
		bool HasFlag(MetroGlobalsFlags flag) const;
		bool Initialize();
		bool InitializeTempDirectory();
		bool InitializeLog();
		bool InitializeGraphics();
		bool InitializeAudio();
		bool InitializePointer();
		bool InitializeKeyboard();
		bool InitializeJoystick();
		bool InitializeRender2d();

		// Properties
		bool _valid;
		double _dpiScale;
		MetroGlobalsFlags _flags;
		MetroGlobalsEvents ^_events;
		Windows::UI::Xaml::Window ^_window;
		Windows::UI::Xaml::Controls::SwapChainPanel ^_swapPanel;
		Windows::Graphics::Display::DisplayInformation ^_displayInfo;

		// Resource context
		ComPtr<IAudioDevice> _audio;
		ComPtr<IGraphDevice> _graph;
		ComPtr<IRenderTargetWindow> _target;
		ComPtr<IRenderDepth> _depth;
		ComPtr<IPointerDevice> _pointer;
		ComPtr<IKeyboardDevice> _keyboard;
		ComPtr<IJoystickInput> _joysticks;
		ComPtr<I2dRenderer> _render2d;
		ComPtr<I2dEffect> _effect2d;

		// Log
		String _logFile;
		ComPtr<IDataWriter> _logWriter;

		// State
		Mutex _stateLock;
		Map<String, Dict> _namedState;
		Dict _namedStateCache;
		bool _stateChanged;

		// Game loop
		WinHandle _gameLoopEvent;
		std::shared_ptr<State> _gameState;
		Windows::Foundation::IAsyncAction ^_gameLoopAction;
		ComPtr<IThreadDispatch> _gameLoopDispatch;
		InitialStateFactory _initialStateFactory;

		// Game timer
		Timer _timer;
		FrameTime _frameTime;
		GlobalTime _globalTime;
	};
}

#endif
