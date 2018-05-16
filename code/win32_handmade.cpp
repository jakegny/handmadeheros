#include <Windows.h>

#define internal static 
#define local_persist static 
#define global_var static 

// TODO: this is a global for now
global_var bool RUNNING;
global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;
global_var HBITMAP BitmapHandle;
global_var HDC BitmapDeviceContext;

internal void
Win32ResizeDIBSection(int Width, int Height) {

  // TODO: bulletproof this
  // maybe don't free first, free after, then free first if that fails

  if(BitmapHandle) {
    DeleteObject(BitmapHandle);
  } 
  
  if (!BitmapDeviceContext) {
    // TODO: should we recreate these under certain special circumstances?
    BitmapDeviceContext = CreateCompatibleDC(0);
  }

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapHandle = CreateDIBSection(
    BitmapDeviceContext, &BitmapInfo,
    DIB_RGB_COLORS,
    &BitmapMemory,
    0, 0);
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height) {
    StretchDIBits(DeviceContext, 
                  X, Y, Width, Height, // Dest Rect
                  X, Y, Width, Height, // Source Rect
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS , SRCCOPY);
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
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(Width, Height);
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
      Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
      // local_persist DWORD Operation = WHITENESS;
      // PatBlt(DeviceContext, X, Y, Width, Height, Operation);
      // if(Operation == WHITENESS) {
      //   Operation = BLACKNESS;
      // } else {
      //   Operation = WHITENESS;
      // }
      // EndPaint(Window, &Paint);
      
    } break;
    default: {
      // OutputDebugStringA("default\n");
      Result = DefWindowProc(Window, Message, WParam, LParam);
      
    } break;
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
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  // WindowClass.cbWndExtra = ; 
  WindowClass.hInstance = hInstance;
  // WindowsClass.hIcon;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
  
  if (RegisterClass(&WindowClass)) {
    HWND WindowHandle = CreateWindowEx(
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
    if(WindowHandle) {
      RUNNING = true;
      while(RUNNING) { // loops forever
        MSG Message;
        BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
        if(MessageResult > 0) { // could return -1 on failure
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        } else {
          break;
        }
      }
      
    } else {
    // TODO: Logging
    }
  } else {
    // TODO: Logging
  }
  

  return(0);
}