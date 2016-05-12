#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioFactory.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Dict/DictPersist.h"
#include "Globals/GlobalsScope.h"
#include "Globals/Log.h"
#include "Globals/MetroGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Globals/ThreadGlobals.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Input/Joystick/JoystickInput.h"
#include "Input/KeyboardDevice.h"
#include "Input/PointerDevice.h"
#include "State/FpsDisplay.h"
#include "State/States.h"
#include "State/StateWrapper.h"
#include "String/StringUtil.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"
#include "Windows/FileUtil.h"

#if METRO_APP

static ff::MetroGlobals *s_metroGlobals = nullptr;

static void WriteLog(const wchar_t *id, ...)
{
	va_list args;
	va_start(args, id);

	ff::String formatString = ff::String::format_new(L"%s %s: %s\r\n",
		ff::GetDateAsString().c_str(),
		ff::GetTimeAsString().c_str(),
		ff::GetThisModule().GetString(ff::String(id)).c_str());
	ff::Log::GlobalTraceV(formatString.c_str(), args);

	va_end(args);
}

// Listens to WinRT events for MetroGlobals
ref class MetroGlobalsEvents sealed
{
public:
	MetroGlobalsEvents();
	virtual ~MetroGlobalsEvents();

internal:
	bool Startup(ff::MetroGlobals *globals);
	void Shutdown();

private:
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

	ff::MetroGlobals *_globals;
	Windows::Foundation::EventRegistrationToken _tokens[11];
};

MetroGlobalsEvents::MetroGlobalsEvents()
	: _globals(nullptr)
{
}

MetroGlobalsEvents::~MetroGlobalsEvents()
{
}

bool MetroGlobalsEvents::Startup(ff::MetroGlobals *globals)
{
	assertRetVal(globals && !_globals, false);
	_globals = globals;

	_tokens[0] = Windows::ApplicationModel::Core::CoreApplication::Suspending +=
		ref new Windows::Foundation::EventHandler<Windows::ApplicationModel::SuspendingEventArgs ^>(
			this, &MetroGlobalsEvents::OnAppSuspending);

	_tokens[1] = Windows::ApplicationModel::Core::CoreApplication::Resuming +=
		ref new Windows::Foundation::EventHandler<Platform::Object ^>(
			this, &MetroGlobalsEvents::OnAppResuming);

	_tokens[2] = _globals->GetWindow()->Activated +=
		ref new Windows::UI::Xaml::WindowActivatedEventHandler(this, &MetroGlobalsEvents::OnWindowActivated);

	_tokens[3] = _globals->GetWindow()->VisibilityChanged +=
		ref new Windows::UI::Xaml::WindowVisibilityChangedEventHandler(this, &MetroGlobalsEvents::OnVisibilityChanged);

	_tokens[4] = _globals->GetWindow()->Closed +=
		ref new Windows::UI::Xaml::WindowClosedEventHandler(this, &MetroGlobalsEvents::OnWindowClosed);

	_tokens[5] = _globals->GetWindow()->SizeChanged +=
		ref new Windows::UI::Xaml::WindowSizeChangedEventHandler(this, &MetroGlobalsEvents::OnSizeChanged);

	_tokens[6] = _globals->GetDisplayInfo()->DpiChanged +=
		ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation ^, Platform::Object ^>(
			this, &MetroGlobalsEvents::OnLogicalDpiChanged);

	_tokens[7] = _globals->GetDisplayInfo()->DisplayContentsInvalidated +=
		ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation ^, Platform::Object ^>(
			this, &MetroGlobalsEvents::OnDisplayContentsInvalidated);

	_tokens[8] = _globals->GetDisplayInfo()->OrientationChanged +=
		ref new Windows::Foundation::TypedEventHandler<Windows::Graphics::Display::DisplayInformation ^, Platform::Object ^>(
			this, &MetroGlobalsEvents::OnOrientationChanged);

	if (_globals->GetSwapChainPanel())
	{
		_tokens[9] = _globals->GetSwapChainPanel()->CompositionScaleChanged +=
			ref new Windows::Foundation::TypedEventHandler<Windows::UI::Xaml::Controls::SwapChainPanel ^, Platform::Object ^>(
				this, &MetroGlobalsEvents::OnCompositionScaleChanged);

		_tokens[10] = _globals->GetSwapChainPanel()->SizeChanged +=
			ref new Windows::UI::Xaml::SizeChangedEventHandler(
				this, &MetroGlobalsEvents::OnSwapChainSizeChanged);
	}

	return true;
}

void MetroGlobalsEvents::Shutdown()
{
	noAssertRet(_globals);

	Windows::ApplicationModel::Core::CoreApplication::Suspending -= _tokens[0];
	Windows::ApplicationModel::Core::CoreApplication::Resuming -= _tokens[1];

	_globals->GetWindow()->Activated -= _tokens[2];
	_globals->GetWindow()->VisibilityChanged -= _tokens[3];
	_globals->GetWindow()->Closed -= _tokens[4];
	_globals->GetWindow()->SizeChanged -= _tokens[5];
	_globals->GetDisplayInfo()->DpiChanged -= _tokens[6];
	_globals->GetDisplayInfo()->DisplayContentsInvalidated -= _tokens[7];
	_globals->GetDisplayInfo()->OrientationChanged -= _tokens[8];

	if (_globals->GetSwapChainPanel())
	{
		_globals->GetSwapChainPanel()->CompositionScaleChanged -= _tokens[9];
		_globals->GetSwapChainPanel()->SizeChanged -= _tokens[10];
	}

	_globals = nullptr;

	ff::ZeroObject(_tokens);
}

void MetroGlobalsEvents::OnAppSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^args)
{
	assertRet(_globals);
	_globals->OnAppSuspending(sender, args);
}

void MetroGlobalsEvents::OnAppResuming(Platform::Object ^sender, Platform::Object ^arg)
{
	assertRet(_globals);
	_globals->OnAppResuming(sender, arg);
}

void MetroGlobalsEvents::OnWindowActivated(Platform::Object ^sender, Windows::UI::Core::WindowActivatedEventArgs ^args)
{
	assertRet(_globals);
	_globals->OnWindowActivated(sender, args);
}

void MetroGlobalsEvents::OnVisibilityChanged(Platform::Object ^sender, Windows::UI::Core::VisibilityChangedEventArgs ^args)
{
	assertRet(_globals);
	_globals->OnVisibilityChanged(sender, args);
}

void MetroGlobalsEvents::OnWindowClosed(Platform::Object ^sender, Windows::UI::Core::CoreWindowEventArgs ^args)
{
	assertRet(_globals);
	_globals->OnWindowClosed(sender, args);
}

void MetroGlobalsEvents::OnSizeChanged(Platform::Object ^sender, Windows::UI::Core::WindowSizeChangedEventArgs ^args)
{
	assertRet(_globals);
	_globals->OnSizeChanged(sender, args);
}

void MetroGlobalsEvents::OnLogicalDpiChanged(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender)
{
	assertRet(_globals);
	_globals->OnLogicalDpiChanged(displayInfo, sender);
}

void MetroGlobalsEvents::OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender)
{
	assertRet(_globals);
	_globals->OnDisplayContentsInvalidated(displayInfo, sender);
}

void MetroGlobalsEvents::OnOrientationChanged(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender)
{
	assertRet(_globals);
	_globals->OnOrientationChanged(displayInfo, sender);
}

void MetroGlobalsEvents::OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^panel, Platform::Object ^sender)
{
	assertRet(_globals);
	_globals->OnCompositionScaleChanged(panel, sender);
}

void MetroGlobalsEvents::OnSwapChainSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^args)
{
	assertRet(_globals);
	_globals->OnSwapChainSizeChanged(sender, args);
}

ff::MetroGlobals::MetroGlobals()
	: _valid(false)
	, _dpiScale(1)
	, _flags(MetroGlobalsFlags::None)
	, _stateChanged(false)
{
	assert(ff::ProcessGlobals::Exists() && !s_metroGlobals);
	s_metroGlobals = this;
}

ff::MetroGlobals::~MetroGlobals()
{
	assert(!_valid && s_metroGlobals == this);
	s_metroGlobals = nullptr;
}

ff::AppGlobals *ff::AppGlobals::Get()
{
	return s_metroGlobals;
}

ff::MetroGlobals *ff::MetroGlobals::Get()
{
	return s_metroGlobals;
}

bool ff::MetroGlobals::Startup(
	MetroGlobalsFlags flags,
	Windows::UI::Xaml::Window ^window,
	Windows::UI::Xaml::Controls::SwapChainPanel ^swapPanel,
	Windows::Graphics::Display::DisplayInformation ^displayInfo,
	FinishInitFunc finishInitFunc,
	InitialStateFactory initialStateFactory)
{
	assertRetVal(ff::DidProgramStart(), false);
	assertRetVal(!_valid && window && displayInfo && !window->Visible, false);

	_flags = flags;
	_window = window;
	_swapPanel = swapPanel;
	_displayInfo = displayInfo;
	_gameLoopEvent = ff::CreateEvent();
	_initialStateFactory = initialStateFactory;

	assertRetVal(Initialize(), false);
	assertRetVal(finishInitFunc == nullptr || finishInitFunc(this), false);

	::WriteLog(L"APP_STARTUP_DONE");
	return _valid = true;
}

void ff::MetroGlobals::Shutdown()
{
	::WriteLog(L"APP_SHUTDOWN");
	RunRenderLoop(false);

	_valid = false;
	_window = nullptr;
	_displayInfo = nullptr;
	_gameLoopEvent = nullptr;

	_audio = nullptr;
	_effect2d = nullptr;
	_render2d = nullptr;
	_target = nullptr;
	_depth = nullptr;
	_graph = nullptr;
	_pointer = nullptr;
	_keyboard = nullptr;
	_joysticks = nullptr;

	if (_events)
	{
		_events->Shutdown();
		_events = nullptr;
	}

	::WriteLog(L"APP_SHUTDOWN_DONE");

	if (_logWriter)
	{
		ff::ProcessGlobals::Get()->GetLog().RemoveWriter(_logWriter);
		_logWriter = nullptr;
	}
}

ff::IRenderTargetWindow *ff::MetroGlobals::GetTarget() const
{
	return _target;
}

ff::IRenderDepth *ff::MetroGlobals::GetDepth() const
{
	return _depth;
}

ff::IGraphDevice *ff::MetroGlobals::GetGraph() const
{
	return _graph;
}

ff::IAudioDevice *ff::MetroGlobals::GetAudio() const
{
	return _audio;
}

ff::IPointerDevice *ff::MetroGlobals::GetPointer() const
{
	return _pointer;
}

ff::IKeyboardDevice *ff::MetroGlobals::GetKeys() const
{
	return _keyboard;
}

ff::IJoystickInput *ff::MetroGlobals::GetJoysticks() const
{
	return _joysticks;
}

ff::I2dRenderer *ff::MetroGlobals::Get2dRender() const
{
	return _render2d;
}

ff::I2dEffect *ff::MetroGlobals::Get2dEffect() const
{
	return _effect2d;
}

const ff::GlobalTime &ff::MetroGlobals::GetGlobalTime() const
{
	return _globalTime;
}

const ff::FrameTime &ff::MetroGlobals::GetFrameTime() const
{
	return _frameTime;
}

void ff::MetroGlobals::PostGameEvent(hash_t eventId, int data)
{
	GetGameDispatch()->Post([this, eventId, data]()
	{
		if (_gameState != nullptr)
		{
			_gameState->Notify(eventId, data, nullptr);
		}
	});
}

bool ff::MetroGlobals::SendGameEvent(hash_t eventId, int data1, void *data2)
{
	ff::IThreadDispatch *dispatch = GetGameDispatch();

	if (dispatch->IsCurrentThread())
	{
		return (_gameState != nullptr) && _gameState->Notify(eventId, data1, data2);
	}
	else
	{
		bool result = false;

		dispatch->Send([this, eventId, data1, data2, &result]()
		{
			if (_gameState != nullptr)
			{
				result = _gameState->Notify(eventId, data1, data2);
			}
		});

		return result;
	}
}

ff::String ff::MetroGlobals::GetLogFile() const
{
	return _logFile;
}

Windows::UI::Xaml::Window ^ff::MetroGlobals::GetWindow() const
{
	return _window;
}

Windows::UI::Xaml::Controls::SwapChainPanel ^ff::MetroGlobals::GetSwapChainPanel() const
{
	return _swapPanel;
}

Windows::Graphics::Display::DisplayInformation ^ff::MetroGlobals::GetDisplayInfo() const
{
	return _displayInfo;
}

double ff::MetroGlobals::GetDpiScale() const
{
	return _dpiScale;
}

double ff::MetroGlobals::PixelToDip(double size) const
{
	return size / _dpiScale;
}

double ff::MetroGlobals::DipToPixel(double size) const
{
	return size * _dpiScale;
}

bool ff::MetroGlobals::LoadState()
{
	ff::LockMutex lock(_stateLock);
	ClearAllState();

	String fileName = GetStateFile();
	if (ff::FileExists(fileName))
	{
		::WriteLog(L"APP_STATE_LOADING", fileName.c_str());

		ComPtr<IDataFile> dataFile;
		ComPtr<IDataReader> dataReader;
		bool valid =
			ff::CreateDataFile(fileName, false, &dataFile) &&
			ff::CreateDataReader(dataFile, 0, &dataReader) &&
			ff::LoadDict(dataReader, _namedStateCache);
		assert(valid);
	}

	_stateChanged = false;
	return true;
}

bool ff::MetroGlobals::SaveState()
{
	if (_gameState != nullptr)
	{
		_gameState->SaveState(this);
	}

	ff::LockMutex lock(_stateLock);
	noAssertRetVal(_stateChanged, true);

	String fileName = GetStateFile();
	::WriteLog(L"APP_STATE_SAVING", fileName.c_str());

	Dict state = _namedStateCache;
	for (const KeyValue<String, Dict> &kvp : _namedState)
	{
		Dict namedDict = kvp.GetValue();
		if (!namedDict.IsEmpty())
		{
			state.SetDict(kvp.GetKey(), namedDict);
		}
	}

	File file;
	ComPtr<IData> data;
	assertRetVal(ff::SaveDict(state, &data), false);
	assertRetVal(file.OpenWrite(fileName), false);
	assertRetVal(ff::WriteFile(file, data), false);

	_stateChanged = false;
	return true;
}

ff::Dict ff::MetroGlobals::GetState()
{
	return GetState(ff::GetEmptyString());
}

void ff::MetroGlobals::SetState(const Dict &dict)
{
	return SetState(ff::GetEmptyString(), dict);
}

ff::Dict ff::MetroGlobals::GetState(StringRef name)
{
	ff::LockMutex lock(_stateLock);
	BucketIter namedIter = _namedState.Get(name);
	if (namedIter == INVALID_ITER)
	{
		Dict state;
		if (_namedStateCache.GetValue(name))
		{
			state = _namedStateCache.GetDict(name);
		}

		namedIter = _namedState.SetKey(name, state);
		ff::DumpDict(name, state, &ff::ProcessGlobals::Get()->GetLog(), true);
	}

	return _namedState.ValueAt(namedIter);
}

void ff::MetroGlobals::SetState(StringRef name, const Dict &dict)
{
	ff::LockMutex lock(_stateLock);
	_namedStateCache.SetValue(name, nullptr);
	_stateChanged = true;

	if (dict.IsEmpty())
	{
		_namedState.DeleteKey(name);
	}
	else
	{
		_namedState.SetKey(name, dict);
	}
}

void ff::MetroGlobals::ClearAllState()
{
	ff::LockMutex lock(_stateLock);

	if (_namedState.Size() || _namedStateCache.Size())
	{
		_namedState.Clear();
		_namedStateCache.Clear();
		_stateChanged = true;
	}
}

ff::String ff::MetroGlobals::GetStateFile() const
{
	String path = ff::GetRoamingUserDirectory();
	assertRetVal(!path.empty(), path);

	String appName = ff::GetMainModule()->GetName();
	String appArch = GetThisModule().GetBuildArch();
	String name = GetThisModule().GetFormattedString(
		String(L"APP_STATE_FILE"),
		appName.c_str(),
		appArch.c_str());
	AppendPathTail(path, name);

	return path;
}

bool ff::MetroGlobals::HasFlag(MetroGlobalsFlags flag) const
{
	unsigned int intFlag = (unsigned int)_flags;
	unsigned int intCheck = (unsigned int)flag;

	return (intFlag & intCheck) == intCheck;
}

bool ff::MetroGlobals::Initialize()
{
	UpdateDpiScale();

	assertRetVal(InitializeTempDirectory(), false);
	assertRetVal(InitializeLog(), false);
	assertRetVal(InitializeGraphics(), false);
	assertRetVal(InitializeAudio(), false);
	assertRetVal(InitializePointer(), false);
	assertRetVal(InitializeKeyboard(), false);
	assertRetVal(InitializeJoystick(), false);
	assertRetVal(InitializeRender2d(), false);
	assertRetVal(LoadState(), false);

	_events = ref new MetroGlobalsEvents();
	assertRetVal(_events->Startup(this), false);

	return true;
}

bool ff::MetroGlobals::InitializeTempDirectory()
{
	Vector<String> tempDirs, tempFiles;
	if (ff::GetDirectoryContents(ff::GetTempDirectory(), tempDirs, tempFiles))
	{
		for (StringRef tempDir : tempDirs)
		{
			String unusedTempPath = ff::GetTempDirectory();
			ff::AppendPathTail(unusedTempPath, tempDir);

			ff::GetThreadPool()->Add([unusedTempPath]()
			{
				ff::DeleteDirectory(unusedTempPath, false);
			});
		}
	}

	String subDir = String::format_new(L"%u", ::GetCurrentProcessId());
	ff::SetTempSubDirectory(subDir);

	return true;
}

bool ff::MetroGlobals::InitializeLog()
{
	String fullPath = ff::GetTempDirectory();
	ff::AppendPathTail(fullPath, String(L"log.txt"));

	ComPtr<IDataFile> file;
	ComPtr<IDataWriter> writer;
	if (ff::CreateDataFile(fullPath, false, &file) &&
		ff::CreateDataWriter(file, 0, &writer))
	{
		_logFile = fullPath;
		_logWriter = writer;

		ff::WriteUnicodeBOM(writer);
		ff::ProcessGlobals::Get()->GetLog().AddWriter(writer);
	}

	::WriteLog(L"APP_STARTUP");

	return true;
}

bool ff::MetroGlobals::InitializeGraphics()
{
	noAssertRetVal(HasFlag(MetroGlobalsFlags::UseGraphics), true);

	::WriteLog(L"APP_INIT_DIRECT3D");
	assertRetVal(ff::ProcessGlobals::Get()->GetGraphicFactory()->CreateDevice(nullptr, &_graph), false);

	::WriteLog(L"APP_INIT_RENDER_TARGET");
	assertRetVal(ff::CreateRenderTargetWindow(_graph, _window, false, DXGI_FORMAT_UNKNOWN, 0, 0, &_target), false);

	::WriteLog(L"APP_INIT_DEPTH_BUFFER");
	assertRetVal(ff::CreateRenderDepth(_graph, _target->GetBufferSize(), &_depth), false);

	return true;
}

bool ff::MetroGlobals::InitializeAudio()
{
	noAssertRetVal(HasFlag(MetroGlobalsFlags::UseAudio), true);

	::WriteLog(L"APP_INIT_AUDIO");
	assertRetVal(ff::ProcessGlobals::Get()->GetAudioFactory()->CreateDefaultDevice(&_audio), false);

	return true;
}

bool ff::MetroGlobals::InitializePointer()
{
	noAssertRetVal(HasFlag(MetroGlobalsFlags::UsePointer), true);

	::WriteLog(L"APP_INIT_POINTER");
	assertRetVal(ff::CreatePointerDevice(_window, &_pointer), false);

	return true;
}

bool ff::MetroGlobals::InitializeKeyboard()
{
	noAssertRetVal(HasFlag(MetroGlobalsFlags::UseKeyboard), true);

	::WriteLog(L"APP_INIT_KEYBOARD");
	assertRetVal(ff::CreateKeyboardDevice(_window, &_keyboard), false);

	return true;
}

bool ff::MetroGlobals::InitializeJoystick()
{
	noAssertRetVal(HasFlag(MetroGlobalsFlags::UseJoystick), true);

	::WriteLog(L"APP_INIT_JOYSTICKS");
	assertRetVal(ff::CreateJoystickInput(_window, &_joysticks), false);

	return true;
}

bool ff::MetroGlobals::InitializeRender2d()
{
	noAssertRetVal(HasFlag(MetroGlobalsFlags::UseRender2d), true);

	::WriteLog(L"APP_INIT_RENDER_2D");
	assertRetVal(ff::Create2dRenderer(_graph, &_render2d), false);
	assertRetVal(ff::CreateDefault2dEffect(_graph, &_effect2d), false);

	return true;
}

void ff::MetroGlobals::OnAppSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^args)
{
	::WriteLog(L"APP_STATE_SUSPENDING");

	Windows::ApplicationModel::SuspendingDeferral ^deferral = args->SuspendingOperation->GetDeferral();
	ComPtr<IGraphDevice> graph = _graph;

	ff::GetMainThreadDispatch()->Post([deferral, this]()
	{
		RunRenderLoop(false);

		if (_graph)
		{
			_graph->GetContext()->ClearState();
			_graph->GetDXGI()->Trim();
		}

		deferral->Complete();
	});
}

void ff::MetroGlobals::OnAppResuming(Platform::Object ^sender, Platform::Object ^arg)
{
	::WriteLog(L"APP_STATE_RESUMING");
	RunRenderLoop(true);
}

void ff::MetroGlobals::OnWindowActivated(Platform::Object ^sender, Windows::UI::Core::WindowActivatedEventArgs ^args)
{
	RunRenderLoop(_window->Visible);
}

void ff::MetroGlobals::OnVisibilityChanged(Platform::Object ^sender, Windows::UI::Core::VisibilityChangedEventArgs ^args)
{
	RunRenderLoop(args->Visible);
}

void ff::MetroGlobals::OnWindowClosed(Platform::Object ^sender, Windows::UI::Core::CoreWindowEventArgs ^args)
{
}

void ff::MetroGlobals::OnSizeChanged(Platform::Object ^sender, Windows::UI::Core::WindowSizeChangedEventArgs ^args)
{
	// only care about the swap chain changing size
}

void ff::MetroGlobals::OnLogicalDpiChanged(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender)
{
	UpdateDpiScale();
	UpdateSwapChain();
}

void ff::MetroGlobals::OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender)
{
	ValidateGraphDevice();
}

void ff::MetroGlobals::OnOrientationChanged(Windows::Graphics::Display::DisplayInformation ^displayInfo, Platform::Object ^sender)
{
	UpdateSwapChain();
}

void ff::MetroGlobals::OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^panel, Platform::Object ^sender)
{
	UpdateSwapChain();
}

void ff::MetroGlobals::OnSwapChainSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^args)
{
	UpdateSwapChain();
}

void ff::MetroGlobals::StartRenderLoop()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	if (_gameLoopDispatch)
	{
		_gameLoopDispatch->Post([this]()
		{
			FrameResetTimer();
		});
	}
	else if (HasFlag(MetroGlobalsFlags::UseGameLoop))
	{
		::WriteLog(L"APP_RENDER_START");

		if (_gameState == nullptr)
		{
			std::shared_ptr<States> states = std::make_shared<States>();
			states->AddBottom(std::make_shared<FpsDisplay>());
			_gameState = std::make_shared<StateWrapper>(states);

			if (_initialStateFactory != nullptr)
			{
				states->AddBottom(_initialStateFactory(this));
			}
		}

		_gameState->LoadState(this);

		auto workItem = [this](Windows::Foundation::IAsyncAction ^action)
		{
			RenderLoopThread();
		};

		auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler(workItem);

		_gameLoopAction = Windows::System::Threading::ThreadPool::RunAsync(
			workItemHandler,
			Windows::System::Threading::WorkItemPriority::High,
			Windows::System::Threading::WorkItemOptions::TimeSliced);

		ff::WaitForEventAndReset(_gameLoopEvent);
	}
}

void ff::MetroGlobals::StopRenderLoop()
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());
	noAssertRet(_gameLoopAction);

	::WriteLog(L"APP_RENDER_STOP");

	_gameLoopAction->Cancel();
	ff::WaitForEventAndReset(_gameLoopEvent);

	_gameLoopAction = nullptr;
}

void ff::MetroGlobals::RunRenderLoop(bool run)
{
	assert(ff::GetMainThreadDispatch()->IsCurrentThread());

	if (run)
	{
		StartRenderLoop();
	}
	else
	{
		StopRenderLoop();
		SaveState();
	}
}

void ff::MetroGlobals::RenderLoopThread()
{
	assert(!ff::GetMainThreadDispatch()->IsCurrentThread());

	ff::ThreadGlobalsScope<ff::ThreadGlobals> threadScope;
	_gameLoopDispatch = threadScope.GetGlobals().GetDispatch();

	FrameResetTimer();
	::SetEvent(_gameLoopEvent);

	while (_gameLoopAction == nullptr || _gameLoopAction->Status == Windows::Foundation::AsyncStatus::Started)
	{
		if (_gameState->GetStatus() == State::Status::Dead)
		{
			break;
		}

		if (FrameAdvanceAndRender())
		{
			if (_target && !_target->Present(true))
			{
				ValidateGraphDevice();
			}
		}
		else if (_target)
		{
			_target->WaitForVsync();
		}

		_gameLoopDispatch->Flush();
	}

	_gameLoopDispatch->Flush();
	_gameLoopDispatch = nullptr;

	::SetEvent(_gameLoopEvent);
}

void ff::MetroGlobals::UpdateSwapChain()
{
	assertRet(ff::GetMainThreadDispatch()->IsCurrentThread());
	noAssertRet(_target && _depth && _swapPanel != nullptr);

	ComPtr<IRenderTargetWindow> target = _target;
	ComPtr<IRenderDepth> depth = _depth;

	PointInt windowSize = ff::GetClientSize(_window);
	PointDouble panelScale(_swapPanel->CompositionScaleX, _swapPanel->CompositionScaleY);
	PointDouble panelSize(_swapPanel->ActualWidth, _swapPanel->ActualHeight);
	Windows::Graphics::Display::DisplayOrientations nativeOrientation = _displayInfo->NativeOrientation;
	Windows::Graphics::Display::DisplayOrientations currentOrientation = _displayInfo->CurrentOrientation;

	GetGameDispatch()->Post([=]()
	{
		target->UpdateSwapChain(
			windowSize,
			(panelSize * panelScale).ToInt(),
			panelScale,
			nativeOrientation,
			currentOrientation);

		depth->SetSize(target->GetBufferSize());
	}, true);
}

void ff::MetroGlobals::UpdateDpiScale()
{
	assertRet(ff::GetMainThreadDispatch()->IsCurrentThread());

	_dpiScale = _displayInfo->LogicalDpi / 96.0;
}

void ff::MetroGlobals::ValidateGraphDevice()
{
	if (_graph)
	{
		ComPtr<IGraphDevice> graph = _graph;

		GetGameDispatch()->Post([graph]()
		{
			graph->ResetIfNeeded();
		}, true);
	}
}

ff::IThreadDispatch *ff::MetroGlobals::GetGameDispatch() const
{
	return _gameLoopDispatch
		? _gameLoopDispatch
		: ff::GetMainThreadDispatch();
}

bool ff::MetroGlobals::FrameAdvanceAndRender()
{
	FrameStartTimer();

	while (FrameAdvanceTimer())
	{
		if (_frameTime._advanceCount > 1)
		{
			_gameLoopDispatch->Flush();
		}

		FrameAdvanceResources();
		_gameState->Advance(this);

		if (_frameTime._advanceCount > 0)
		{
			_frameTime._advanceTime[_frameTime._advanceCount - 1] = _timer.GetCurrentStoredRawTime();
		}
	}

	return FrameRender();
}

void ff::MetroGlobals::FrameAdvanceResources()
{
	if (_keyboard)
	{
		_keyboard->Advance();
	}

	if (_pointer)
	{
		_pointer->Advance();
	}

	if (_joysticks)
	{
		_joysticks->Advance();
	}

	if (_audio)
	{
		_audio->AdvanceEffects();
	}
}

bool ff::MetroGlobals::FrameRender()
{
	noAssertRetVal(_target, false);

	_target->Clear();
	_depth->Clear();
	_gameState->Render(this, _target);
	_depth->Discard();

	_frameTime._renderTime = _timer.GetCurrentStoredRawTime();
	_globalTime._renderCount++;

	return true;
}

void ff::MetroGlobals::FrameResetTimer()
{
	_timer.Tick();
	_timer.StoreLastTickTime();
}

void ff::MetroGlobals::FrameStartTimer()
{
	const double timeScale = 1.0;
	_timer.SetTimeScale(timeScale);

	_globalTime._absoluteSeconds = _timer.GetSeconds();
	_globalTime._bankSeconds += _timer.Tick();

	_frameTime._advanceCount = 0;
	_frameTime._flipTime = _timer.GetLastTickStoredRawTime();
}

bool ff::MetroGlobals::FrameAdvanceTimer()
{
	if (_globalTime._bankSeconds < _globalTime._secondsPerAdvance)
	{
		return false;
	}

#ifdef _DEBUG
	const size_t maxAdvances = !::IsDebuggerPresent()
		? FrameTime::MAX_ADVANCE_COUNT
		: 1;
#else
	const size_t maxAdvances = FrameTime::MAX_ADVANCE_COUNT;
#endif

	if (_frameTime._advanceCount >= maxAdvances)
	{
		// The game is running way too slow
		_globalTime._bankSeconds = std::fmod(_globalTime._bankSeconds, _globalTime._secondsPerAdvance);
		_globalTime._bankScale = _globalTime._bankSeconds / _globalTime._secondsPerAdvance;
		return false;
	}
	else
	{
		_globalTime._absoluteSeconds += _globalTime._secondsPerAdvance;
		_globalTime._bankSeconds -= _globalTime._secondsPerAdvance;
		_globalTime._bankScale = _globalTime._bankSeconds / _globalTime._secondsPerAdvance;
		_globalTime._advanceCount++;
		_frameTime._advanceCount++;
		return true;
	}
}

#else

ff::AppGlobals *ff::AppGlobals::Get()
{
	return nullptr;
}

#endif
