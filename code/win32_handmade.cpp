#include <Windows.h>

LRESULT CALLBACK
MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam) 
{
  LRESULT Result = 0;
  switch(Message) {
    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
      break;
    }
    case WM_DESTROY:
    {
      OutputDebugStringA("WM_DESTROY\n");
      break;
    }
    case WM_CLOSE:
    {
      OutputDebugStringA("WM_CLOSE\n");
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
      static DWORD Operation = WHITENESS;
      PatBlt(DeviceContext, X, Y, Width, Height, Operation);
      if(Operation == WHITENESS) {
        Operation = BLACKNESS;
      } else {
        Operation = WHITENESS;
      }
      EndPaint(Window, &Paint);
      break;
    }
    default: {
      // OutputDebugStringA("default\n");
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
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
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
      MSG Message;
      for(;;) { // loops forever
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