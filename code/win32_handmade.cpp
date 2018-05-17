#include <Windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static 
#define local_persist static 
#define global_var static 

typedef int32_t bool32;

typedef float real32;
typedef double real64;

// typedef uint8_t uint8;

struct win32_offscreen_buffer {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct win32_window_dimension {
  int Width;
  int Height;
};

// TODO: this is a global for now
global_var bool GlobalRunning;
global_var win32_offscreen_buffer GlobalBackbuffer;
global_var LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// NOTE: Support for XInputGetState 
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
  return(ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: Support for XInputGetState 
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
  return(ERROR_DEVICE_NOT_CONNECTED);
}
global_var x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN *pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
Win32LoadXInput(void) {
  // TODO: Test this on Windows 8
  HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
  if(!XInputLibrary) {
    XInputLibrary = LoadLibrary("xinput1_3.dll");
  }
  if(XInputLibrary) {
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

internal void
Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize) {
  // NOTE: Load the library
  HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

  if(DSoundLibrary) {
    // NOTE: Get a DirectSound object - cooperative
    direct_sound_create *DirectSoundCreate = 
        (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    
    // TODO: Double check this works on XP - DirectSound8 or 7?
    LPDIRECTSOUND DirectSound;
    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8 ;
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;

      if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
        // NOTE: Create a primary buffer
        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
        // BufferDescription.dwBufferBytes = BufferSize;
        // TODO: DSBCAPS_GLOBALFOCUS?

        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
          HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
          if(SUCCEEDED(Error)){
            OutputDebugString("Primary Buffer was set.\n");

            // NOTE: We have finally set the format!
          } else  {
           // TODO: Diagnostic / Logging
          }
        } else  {
          // TODO: Diagnostic / Logging
        }
      } else  {
        // TODO: Diagnostic / Logging
      }

      DSBUFFERDESC BufferDescription = {};
      BufferDescription.dwSize = sizeof(BufferDescription);
      BufferDescription.dwFlags = 0;
      BufferDescription.dwBufferBytes = BufferSize;
      BufferDescription.lpwfxFormat = &WaveFormat;
      HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
      if(SUCCEEDED(Error)) {
        OutputDebugString("Secondary Buffer was set.\n");
      }

      // NOTE: Create a secondary buffer

        // NOTE: Start it playing
    } else {
      // TODO: Diagnostic / Logging
    }
  } else {
    // TODO: Diagnostic / Logging
  }


}

internal win32_window_dimension Win32GetWindowDimension(HWND Window) {
  win32_window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return (Result);
}

internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8_t *Row = (uint8_t *)Buffer->Memory;
  for(int Y = 0; Y < Buffer->Height; ++Y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for(int X = 0; X < Buffer->Width; ++X) {

      /*
        Memory:  BB GG RR xx
        Register xx RR GG BB

		LITTLE ENDIAN ARCHITECTURE
		bytes loaded out of memory to form a bigger value,
		the FIRST byte goes in the lowest value

        Pixel (32 Bits)

      */

      uint8_t Blue = (X + XOffset);
      uint8_t Green = (Y + YOffset);

      *Pixel++ = ((Green << 8) | Blue); // fancy register bit maths
    }
    Row += Buffer->Pitch;
  }
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {

  // TODO: bulletproof this
  // maybe don't free first, free after, then free first if that fails

  if(Buffer->Memory) {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }
  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel; 
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

  // TODO: probably clear this to black?
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, 
                           int WindowWidth, int WindowHeight) {
    // TODO: Aspect Ratio Correction
    StretchDIBits(DeviceContext, 
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam) 
{
  LRESULT Result = 0;
  switch(Message) {
    case WM_CLOSE:
    {
      // TODO: handle this with a message to the user?
      GlobalRunning = false;
      break;
    }
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
      break;
    }
    case WM_DESTROY:
    {
      // TODO: handle this as an error - create window?
      GlobalRunning = false;
      break;
    }
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      // wParam is the virtual keykode
      uint32_t VKCode = WParam;
      bool WasDown = ((LParam & (1 << 30)) != 0);
      bool IsDown = ((LParam & (1 << 31)) == 0);

      if(WasDown != IsDown) {
        if(VKCode == 'W') {
          OutputDebugStringA("W\n");
        }
        else if(VKCode == 'A') {
        }
        else if(VKCode == 'S') {
        }
        else if(VKCode == 'D') {
        }
        else if(VKCode == 'Q') {
        }
        else if(VKCode == 'E') {
        }
        else if(VKCode == VK_UP) {
        }
        else if(VKCode == VK_LEFT) {
        }
        else if(VKCode == VK_DOWN) {
        }
        else if(VKCode == VK_RIGHT) {
        }
        else if(VKCode == VK_ESCAPE) {
          OutputDebugString("Esacpe: ");
          if(IsDown) {
            OutputDebugString("IsDown ");
          }
          if(WasDown) {
            OutputDebugString("WasDown");
          }
          OutputDebugString("\n");

        }
        else if(VKCode == VK_SPACE) {
        }
      }

      bool32 AltKeyIsDown = ((LParam & (1 << 29)) != 0);
      if(VKCode == VK_F4 && AltKeyIsDown) {
        GlobalRunning = false;
      }
      break;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;

      win32_window_dimension Dimension = Win32GetWindowDimension(Window);

      Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, 
                                 Dimension.Width, Dimension.Height);

      EndPaint(Window, &Paint);
      break;
    } 
    default: {
      Result = DefWindowProc(Window, Message, WParam, LParam);
      break;
    } 
  }
  return(Result);
}

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nCmdShow
){

  // Initialization
  Win32LoadXInput();

  // make sure to 0 values
  WNDCLASS WindowClass = {};
  Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

  WindowClass.style = CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = hInstance;
  // WindowsClass.hIcon;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
  
  if (RegisterClass(&WindowClass)) {
    HWND Window = CreateWindowEx(
        0,
        WindowClass.lpszClassName,
        "Handmade Hero",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        hInstance,
        0
    );
    if(Window) {

      HDC DeviceContext = GetDC(Window);

      // Note: Graphics Test
      int xOffset = 0;
      int yOffset = 0;

      // NOTE: For Sound test
      int SamplesPerSecond = 48000;
      int SampleHz = 256;
      int Volume = 3000;
      uint32_t RunningSampleIndex = 0;
      int WavePeriod = SamplesPerSecond/SampleHz;
      int HalfWavePeriod = WavePeriod / 2;
      int BytesPerSample = sizeof(int16_t)*2;
      int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;
      ///////////////////

      // Initialize after we have a window
      Win32InitDSound(Window, SamplesPerSecond, SecondaryBufferSize);
      bool32 SoundIsPlaying = false;

      GlobalRunning = true;
      while(GlobalRunning) {
        MSG Message;
        while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if(Message.message == WM_QUIT) {
            GlobalRunning = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        // TODO: Should we poll this more frequently?
        for (DWORD ControllerIndex=0; ControllerIndex< XUSER_MAX_COUNT; ControllerIndex++ ) {
          XINPUT_STATE ControllerState;
          // ZeroMemory( &state, sizeof(XINPUT_STATE) );
          if( XInputGetState( ControllerIndex, &ControllerState ) == ERROR_SUCCESS ) { // Controller is connected 
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

            int16_t StickX = Pad->sThumbLX;
            int16_t StickY = Pad->sThumbLY;

            xOffset += StickX >> 12;
            yOffset += StickY >> 12;
            if (AButton) {
              yOffset += 2;
            }

          } else {  // Controller is not connected 

          }
        }

        // Gamepad Vibration
        // XINPUT_VIBRATION Vibration;
        // Vibration.wLeftMotorSpeed = 60000;
        // Vibration.wRightMotorSpeed = 60000;
        // XInputSetState(0, &Vibration);

        RenderWeirdGradient(&GlobalBackbuffer, xOffset, yOffset);

        // NOTE: DirectSound output test
        DWORD PlayCursor;
        DWORD WriteCursor;
        if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
          DWORD ByteToLock = RunningSampleIndex * BytesPerSample % SecondaryBufferSize;
          DWORD BytesToWrite;
          // TODO: we need a more accurate check than ByeToLock == PlayCursor
          if(ByteToLock == PlayCursor) {
            BytesToWrite = SecondaryBufferSize;
          } else if(ByteToLock > PlayCursor) {
            BytesToWrite = (SecondaryBufferSize - ByteToLock);
            BytesToWrite += PlayCursor;
          }else {
            BytesToWrite = PlayCursor - ByteToLock;
          }
          // if(ByteToLock > PlayCursor) {
          //   BytesToWrite = (SecondaryBufferSize - ByteToLock);
          //   BytesToWrite += PlayCursor;
          // } else {
          //   BytesToWrite = PlayCursor - ByteToLock;
          // }
          VOID *Region1;
          DWORD Region1Size;
          VOID *Region2;
          DWORD Region2Size;
          if(!SoundIsPlaying && SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                                   &Region1, &Region1Size,
                                                   &Region2, &Region2Size, 0))) {
              // TODO: assert that Region1Size is valid
              DWORD Region1SampleCount = Region1Size/BytesPerSample;
              int16_t *SampleOut = (int16_t *)Region1;
              for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
                int16_t SampleValue = ((RunningSampleIndex++ / HalfWavePeriod) % 2) ? Volume : -Volume;
                *SampleOut++ = SampleValue;
                *SampleOut++ = SampleValue;
              }

              DWORD Region2SampleCount = Region2Size/BytesPerSample;
              SampleOut = (int16_t *)Region2;
              for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {    
                int16_t SampleValue = ((RunningSampleIndex++ / HalfWavePeriod) % 2) ? Volume : -Volume;
                *SampleOut++ = SampleValue;
                *SampleOut++ = SampleValue;
              }

              GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
            }
        }

        if(!SoundIsPlaying) {
          GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
          SoundIsPlaying = true;
        }

        win32_window_dimension Dimension = Win32GetWindowDimension(Window); 
        Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, 
                                   Dimension.Width, Dimension.Height);
        // ReleaseDC(Window, DeviceContext);
      }
      
    } else {
    // TODO: Logging
    }
  } else {
    // TODO: Logging
  }

  return(0);
}