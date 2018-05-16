#include <Windows.h>
#include <stdint.h>
#include <Xinput.h>

#define internal static 
#define local_persist static 
#define global_var static 

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
global_var bool RUNNING;
global_var win32_offscreen_buffer GlobalBackbuffer;

win32_window_dimension Win32GetWindowDimension(HWND Window) {
  win32_window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return (Result);
}

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset) {
  uint8_t *Row = (uint8_t *)Buffer.Memory;
  for(int Y = 0; Y < Buffer.Height; ++Y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for(int X = 0; X < Buffer.Width; ++X) {

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
    Row += Buffer.Pitch;
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
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, 
                           win32_offscreen_buffer Buffer,
                           int X, int Y, int Width, int Height) {
    // TODO: Aspect Ratio Correction
    StretchDIBits(DeviceContext, 
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer.Width, Buffer.Height,
                  Buffer.Memory,
                  &Buffer.Info,
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
    case WM_SIZE:
    {
      // win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      // Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
      break;
    }
    case WM_DESTROY:
    {
      // TODO: handle this as an error - create window?
      RUNNING = false;
      break;
    }
    case WM_CLOSE:
    {
      // TODO: handle this with a message to the user?
      RUNNING = false;
      break;
    }
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
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

      Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, 
                                 GlobalBackbuffer, X, Y, Width, Height);

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
      int xOffset = 0;
      int yOffset = 0;
      RUNNING = true;
      while(RUNNING) {


        MSG Message;
        while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if(Message.message == WM_QUIT) {
            RUNNING = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        // TODO: Should we poll this more frequently?
        for (DWORD ControllerIndex=0; ControllerIndex< XUSER_MAX_COUNT; ControllerIndex++ ) {
          XINPUT_STATE ControllerState;
          // ZeroMemory( &state, sizeof(XINPUT_STATE) );
          if( XInputGetState( ControllerIndex, &ControllerState ) == ERROR_SUCCESS ) { // Controller is connected 
            XINPUT_GAMEPAD = *Pad = &ControllerState.Gamepad;
          } else {  // Controller is not connected 

          }
        }

        RenderWeirdGradient(GlobalBackbuffer, xOffset, yOffset);

        HDC DeviceContext = GetDC(Window);

        win32_window_dimension Dimension = Win32GetWindowDimension(Window); 
        Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, 
                                   0, 0, Dimension.Width, Dimension.Height);
        ReleaseDC(Window, DeviceContext);

        ++xOffset;
        yOffset += 2;
      }
      
    } else {
    // TODO: Logging
    }
  } else {
    // TODO: Logging
  }
  

  return(0);
}