#include <limits.h>
#include "XlibWindow.h"

using namespace vl::collections;

namespace vl
{
	namespace presentation
	{
		namespace x11cairo
		{
			namespace xlib
			{
				XlibWindow::XlibWindow(Display *display):
					display(display),
					title(),
					renderTarget(nullptr),
					resizable (false),
					doubleBuffer(false),
					backBuffer(XLIB_NONE)
				{
					this->display = display;
					window = XCreateWindow(
							display,                 //Display
							XRootWindow(display, 0), //Root Display
							0,                       //X
							0,                       //Y
							400,                     //Width
							200,                     //Height
							2,                       //Border Width
							CopyFromParent,          //Depth
							InputOutput,             //Class
							CopyFromParent,          //Visual
							0,                       //Value Mask
							NULL                     //Attributes
							);

					Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", XLIB_FALSE);

					XSelectInput(display, window, PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | SubstructureNotifyMask);
					XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

					CheckDoubleBuffer();

					Show();
					UpdateTitle();
					XSync(display, false);
				}

				XlibWindow::~XlibWindow()
				{
					delete renderTarget;
					XDestroyWindow(display, window);
				}

				void XlibWindow::CheckDoubleBuffer()
				{
#ifdef GAC_X11_DOUBLEBUFFER
					if(CheckXdbeExtension(display))
					{
						RebuildDoubleBuffer();
						if(backBuffer != XLIB_NONE)
						{
							doubleBuffer = true;
						}
					}
#endif
				}

				void XlibWindow::RebuildDoubleBuffer()
				{
					if(backBuffer != XLIB_NONE)
						XdbeDeallocateBackBufferName(display, backBuffer);
					backBuffer = XdbeAllocateBackBufferName(display, window, XdbeUndefined);
				}

				XdbeBackBuffer XlibWindow::GetBackBuffer()
				{
					return backBuffer;
				}

				bool XlibWindow::GetDoubleBuffer()
				{
					return doubleBuffer;
				}

				void XlibWindow::SwapBuffer()
				{
					if(doubleBuffer)
					{
						XdbeBackBufferAttributes* attributes = XdbeGetBackBufferAttributes(display, backBuffer);
						XdbeSwapInfo info;
						{
							info.swap_window = window;
							info.swap_action = XdbeUndefined;
						}

						XdbeSwapBuffers(display, &info, 1);
						XFree(attributes);
					}
				}

				void XlibWindow::UpdateResizable()
				{
					XSizeHints *hints = XAllocSizeHints();
					Size currentSize = GetClientSize();
					XGetNormalHints(display, window, hints);
					if(resizable)
					{
						hints->min_width = currentSize.x;
						hints->min_height = currentSize.y;
						hints->max_width = currentSize.x;
						hints->max_height = currentSize.y;
					}
					else
					{
						hints->min_width = 0;
						hints->min_height = 0;
						hints->max_width = INT_MAX;
						hints->max_height = INT_MAX;
					}
					XSetNormalHints(display, window, hints);
					XFree(hints);
				}

				Window XlibWindow::GetWindow()
				{
					return window;
				}

				Display* XlibWindow::GetDisplay()
				{
					return display;
				}

				void XlibWindow::SetRenderTarget(elements::IGuiGraphicsRenderTarget* target)
				{
					renderTarget = target;
				}

				elements::IGuiGraphicsRenderTarget* XlibWindow::GetRenderTarget()
				{
					return renderTarget;
				}

				void XlibWindow::UpdateTitle()
				{
					AString narrow = wtoa(title);
					XStoreName(display, window, narrow.Buffer());
				}

				void XlibWindow::ResizeEvent(int width, int height)
				{
					RebuildDoubleBuffer();
					Rect newBound = GetBounds();
					FOREACH(INativeWindowListener*, i, listeners)
					{
						i->Moving(newBound, true);
						i->Moved();
					}
				}

				void XlibWindow::MouseUpEvent(MouseButton button, Point position)
				{
					NativeWindowMouseInfo info;
					{
						info.x = position.x;
						info.y = position.y;
						info.left = (button == MouseButton::LBUTTON) ? true : false;
						info.right = (button == MouseButton::RBUTTON) ? true : false;
						info.shift = false;
						info.ctrl = false;
					}
					switch(button)
					{
						case MouseButton::LBUTTON:
							FOREACH(INativeWindowListener*, i, listeners)
							{
								i->LeftButtonUp(info);
							}
							break;

						case MouseButton::RBUTTON:
							FOREACH(INativeWindowListener*, i, listeners)
							{
								i->RightButtonUp(info);
							}
							break;
					}
				}

				void XlibWindow::MouseDownEvent(MouseButton button, Point position)
				{
					NativeWindowMouseInfo info;
					{
						info.x = position.x;
						info.y = position.y;
						info.left = (button == MouseButton::LBUTTON) ? true : false;
						info.right = (button == MouseButton::RBUTTON) ? true : false;
						info.shift = false;
						info.ctrl = false;
					}
					switch(button)
					{
						case MouseButton::LBUTTON:
							FOREACH(INativeWindowListener*, i, listeners)
							{
								i->LeftButtonDown(info);
							}
							break;

						case MouseButton::RBUTTON:
							FOREACH(INativeWindowListener*, i, listeners)
							{
								i->RightButtonDown(info);
							}
							break;
					}
				}

				void XlibWindow::MouseMoveEvent(Point position)
				{
					NativeWindowMouseInfo info;
					{
						info.x = position.x;
						info.y = position.y;
					}

					FOREACH(INativeWindowListener*, i, listeners)
					{
						i->MouseMoving(info);
					}
				}

				void XlibWindow::MouseEnterEvent()
				{
					FOREACH(INativeWindowListener*, i, listeners)
					{
						i->MouseEntered();
					}
				}

				void XlibWindow::MouseLeaveEvent()
				{
					FOREACH(INativeWindowListener*, i, listeners)
					{
						i->MouseLeaved();
					}
				}

				void XlibWindow::Show ()
				{
					XMapWindow(display, window);
				}

				void XlibWindow::Hide ()
				{
					XUnmapWindow(display, window);
				}

				Rect XlibWindow::GetBounds()
				{
					//TODO
					XWindowAttributes attr;
					XGetWindowAttributes(display, window, &attr);
					return Rect(attr.x, attr.y, attr.x + attr.width, attr.y + attr.height);
				}

				void XlibWindow::SetBounds(const Rect &bounds)
				{
					//TODO
					XMoveResizeWindow(display, window, bounds.x1, bounds.y1, bounds.Width(), bounds.Height());
				}

				Size XlibWindow::GetClientSize()
				{
					XWindowAttributes attr;
					XGetWindowAttributes(display, window, &attr);
					return Size(attr.width, attr.height);
				}

				void XlibWindow::SetClientSize(Size size)
				{
					XResizeWindow(display, window, size.x, size.y);
				}

				Rect XlibWindow::GetClientBoundsInScreen()
				{
					//TODO
					return Rect();
				}

				WString XlibWindow::GetTitle()
				{
					return title;
				}

				void XlibWindow::SetTitle(WString title)
				{
					this->title = title;
					UpdateTitle();
				}

				INativeCursor *XlibWindow::GetWindowCursor()
				{
					//TODO
					return NULL;
				}

				void XlibWindow::SetWindowCursor(INativeCursor *cursor)
				{
					//TODO
				}

				Point XlibWindow::GetCaretPoint()
				{
					//TODO
					return Point();
				}

				void XlibWindow::SetCaretPoint(Point point)
				{
					//TODO
				}

				INativeWindow *XlibWindow::GetParent()
				{
					//TODO
					return NULL;
				}

				void XlibWindow::SetParent(INativeWindow *parent)
				{
					//TODO
				}

				bool XlibWindow::GetAlwaysPassFocusToParent()
				{
					//TODO
					return false;
				}

				void XlibWindow::SetAlwaysPassFocusToParent(bool value)
				{
					//TODO
				}

				void XlibWindow::EnableCustomFrameMode()
				{
					//TODO
				}

				void XlibWindow::DisableCustomFrameMode()
				{
					//TODO
				}

				bool XlibWindow::IsCustomFrameModeEnabled()
				{
					//TODO
					return false;
				}

				XlibWindow::WindowSizeState XlibWindow::GetSizeState()
				{
					//TODO
					return XlibWindow::WindowSizeState::Restored;
				}

				void XlibWindow::ShowDeactivated()
				{
					//TODO
				}

				void XlibWindow::ShowRestored()
				{
					//TODO
				}

				void XlibWindow::ShowMaximized()
				{
					//TODO
				}

				void XlibWindow::ShowMinimized()
				{
					//TODO
				}

				bool XlibWindow::IsVisible()
				{
					//TODO
					return true;
				}

				void XlibWindow::Enable()
				{
					//TODO
				}

				void XlibWindow::Disable()
				{
					//TODO
				}

				bool XlibWindow::IsEnabled()
				{
					//TODO
					return true;
				}

				void XlibWindow::SetFocus()
				{
					//TODO
				}

				bool XlibWindow::IsFocused()
				{
					//TODO
					return true;
				}

				void XlibWindow::SetActivate()
				{
					//TODO
				}

				bool XlibWindow::IsActivated()
				{
					//TODO
					return true;
				}

				void XlibWindow::ShowInTaskBar()
				{
					//TODO
				}

				void XlibWindow::HideInTaskBar()
				{
					//TODO
				}

				bool XlibWindow::IsAppearedInTaskBar()
				{
					//TODO
					return true;
				}

				void XlibWindow::EnableActivate()
				{
					//TODO
				}

				void XlibWindow::DisableActivate()
				{
					//TODO
				}

				bool XlibWindow::IsEnabledActivate()
				{
					//TODO
					return true;
				}

				bool XlibWindow::RequireCapture()
				{
					//TODO
					return false;
				}

				bool XlibWindow::ReleaseCapture()
				{
					//TODO
					return true;
				}

				bool XlibWindow::IsCapturing()
				{
					//TODO
					return false;
				}

				bool XlibWindow::GetMaximizedBox()
				{
					//TODO
					return false;
				}

				void XlibWindow::SetMaximizedBox(bool visible)
				{
					//TODO
				}

				bool XlibWindow::GetMinimizedBox()
				{
					//TODO
					return false;
				}

				void XlibWindow::SetMinimizedBox(bool visible)
				{
					//TODO
				}

				bool XlibWindow::GetBorder()
				{
					//TODO
					return true;
				}

				void XlibWindow::SetBorder(bool visible)
				{
					//TODO
				}

				bool XlibWindow::GetSizeBox()
				{
					return resizable;
				}

				void XlibWindow::SetSizeBox(bool visible)
				{
					resizable = visible;
					UpdateResizable();
				}

				bool XlibWindow::GetIconVisible()
				{
					//TODO
					return true;
				}

				void XlibWindow::SetIconVisible(bool visible)
				{
					//TODO
				}

				bool XlibWindow::GetTitleBar()
				{
					//TODO
					return true;
				}

				void XlibWindow::SetTitleBar(bool visible)
				{
					//TODO
				}

				bool XlibWindow::GetTopMost()
				{
					//TODO
					return false;
				}

				void XlibWindow::SetTopMost(bool topmost)
				{
					//TODO
				}

				void XlibWindow::SupressAlt()
				{
					//TODO
				}

				bool XlibWindow::InstallListener(INativeWindowListener *listener)
				{
					listeners.Add(listener);
					return true;
				}

				bool XlibWindow::UninstallListener(INativeWindowListener *listener)
				{
					return listeners.Remove(listener);
				}

				void XlibWindow::RedrawContent()
				{
					//TODO
				}
			}
		}
	}
}

