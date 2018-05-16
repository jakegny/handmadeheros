#include <Windows.h>
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_var static 

// typedef uint8_t uint8;

// TODO: this is a global for now
global_var bool RUNNING;
global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;
global_var int BitmapWidth;
global_var int BitmapHeight;

internal void
RenderWeirdGradient(int XOffset, int YOffset) {
  int Width = BitmapWidth;
  int Height = BitmapHeight;
  int Pitch = Width * 4; // 4 Bytes per pixel
  // uint8 *Row = (uint8 *)BitmapMemory;
  uint8_t *Row = (uint8_t *)BitmapMemory;
  for(int Y = 0; Y < BitmapHeight; ++Y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for(int X = 0; X < BitmapWidth; ++X) {

      /*
        Memory:  BB GG RR xx
        Register xx RR GG BB

        Pixel (32 Bits)

      */

      uint8_t Blue = (X + XOffset);
      uint8_t Green = (Y + YOffset);

      *Pixel++ = ((Green << 8) | Blue); // fancy register bit maths

      /*
                         +0 +1 +2 +3
        Pixel in memory: 00 00 00 00
						            Pad	 B	G  R
		    0x00BBGGRR

		LITTLE ENDIAN ARCHITECTURE
		bytes loaded out of memory to form a bigger value, 
		the FIRST byte goes in the lowest value

      */
      // NOTE: Manually updating
      // *Pixel = (uint8_t)(X + XOffset);
      // ++Pixel;

      // *Pixel = (uint8_t)(Y + YOffset);
      // ++Pixel;     

      // *Pixel = 0;
      // ++Pixel;

      // *Pixel = 0;
      // ++Pixel;
    }
    Row += Pitch;
  }
}

internal void
Win32ResizeDIBSection(int Width, int Height) {

  // TODO: bulletproof this
  // maybe don't free first, free after, then free first if that fails

  if(BitmapMemory) {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }
  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = (Width * Height) * 4; // 4 Bytes per pixel
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // TODO: probably clear this to black?
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height) {
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;
    StretchDIBits(DeviceContext, 
                  0, 0, BitmapWidth, BitmapHeight,
                  0, 0, WindowWidth, WindowHeight,
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

      RECT ClientRect;
      GetClientRect(Window, &ClientRect);

      Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
      
    } break;
    default: {
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

        RenderWeirdGradient(xOffset, yOffset);

        HDC DeviceContext = GetDC(WindowHandle);
        RECT ClientRect;
        GetClientRect(WindowHandle, &ClientRect);
        int WindowWidth = ClientRect.right - ClientRect.left;
        int WindowHeight = ClientRect.bottom - ClientRect.top;
        Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
        ReleaseDC(WindowHandle, DeviceContext);

        ++xOffset;
      }
      
    } else {
    // TODO: Logging
    }
  } else {
    // TODO: Logging
  }
  

  return(0);
}