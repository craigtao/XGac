#include <limits.h>
#include "XlibAtoms.h"
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
					customFrameMode(false),
					visible(false),
					backBuffer(XLIB_NONE),
					parentWindow(NULL),
					bounds(0, 0, 400, 200),
					clientSize(400, 200)
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


					XSelectInput(display, window, PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | SubstructureNotifyMask | VisibilityChangeMask | ExposureMask);
					XSetWMProtocols(display, window, &XlibAtoms::WM_DELETE_WINDOW, 1);

					CheckDoubleBuffer();

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
							info.swap_action = XdbeCopied;
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

				void XlibWindow::GetParentList(vl::collections::List<Window>& result)
				{
					XlibWindow* win = this;

					while(win)
					{
						result.Add(win->GetWindow());
						win = dynamic_cast<XlibWindow*>(win->GetParent());
					}
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

					GetBounds();
					GetClientSize();
					RedrawContent();
				}

				void XlibWindow::MouseUpEvent(MouseButton button, NativeWindowMouseInfo info)
				{
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
							
						case MouseButton::MBUTTON:
							FOREACH(INativeWindowListener*, i, listeners)
							{
								i->MiddleButtonUp(info);
							}
							break;
					}
				}

				void XlibWindow::MouseDownEvent(MouseButton button, NativeWindowMouseInfo info)
				{
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

						case MouseButton::MBUTTON:
							FOREACH(INativeWindowListener*, i, listeners)
							{
								i->MiddleButtonDown(info);
							}
							break;

					}
				}

				void XlibWindow::MouseMoveEvent(NativeWindowMouseInfo info)
				{
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

				void XlibWindow::VisibilityEvent(Window window)
				{
					if(visible && parentWindow)
					{
						collections::List<Window> windows;
						GetParentList(windows);
						XRestackWindows(display, &windows[0], windows.Count());
					}
				}

				void XlibWindow::Show()
				{
					XMapWindow(display, window);

					visible = true;
					SetBounds(bounds);
					GetClientSize();
				}

				void XlibWindow::Hide()
				{
					XUnmapWindow(display, window);

					visible = false;
				}

				Rect XlibWindow::GetBounds()
				{
					//TODO
					if(visible)
					{
						int x, y;
						Window child;
						XWindowAttributes attr;
						XGetWindowAttributes(display, window, &attr);
						XTranslateCoordinates(display, window, XDefaultRootWindow(display), 0, 0, &x, &y, &child);
						
						bounds = Rect(x, y, x + attr.width, y + attr.height);
					}

					return bounds;
				}

				void XlibWindow::SetBounds(const Rect &bounds)
				{
					//TODO
					this->bounds = bounds;
					if(visible)
						XMoveResizeWindow(display, window, bounds.x1, bounds.y1, bounds.Width(), bounds.Height());
				}

				Size XlibWindow::GetClientSize()
				{
					if(visible)
					{
						XWindowAttributes attr;
						XGetWindowAttributes(display, window, &attr);
						clientSize = Size(attr.width, attr.height);
					}

					return clientSize;
				}

				void XlibWindow::SetClientSize(Size size)
				{
					clientSize = size;
					if(visible)
						XResizeWindow(display, window, size.x, size.y);
				}

				Rect XlibWindow::GetClientBoundsInScreen()
				{
					//TODO
					return GetBounds();
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
					return parentWindow;
				}

				void XlibWindow::SetParent(INativeWindow *parent)
				{
					parentWindow = dynamic_cast<XlibWindow*>(parent);

					if(parentWindow)
					{
						XChangeProperty(display, window, XlibAtoms::_NET_WM_WINDOW_TYPE, XlibAtoms::_NET_WM_WINDOW_TYPE, 32, PropModeReplace,
								(unsigned char*)&XlibAtoms::_NET_WM_WINDOW_TYPE_POPUP_MENU, 1);
					}
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
					MotifWmHints hints;

					hints.flags = 1 << 1;
					hints.decorations = 0;
					
					XChangeProperty(display, window, XlibAtoms::_MOTIF_WM_HINTS, XlibAtoms::_MOTIF_WM_HINTS, 32, PropModeReplace, (unsigned char*) &hints, 5);

					customFrameMode = true;
				}

				void XlibWindow::DisableCustomFrameMode()
				{
					MotifWmHints hints;

					hints.flags = 1 << 1;
					hints.decorations = 1;
					
					XChangeProperty(display, window, XlibAtoms::_MOTIF_WM_HINTS, XlibAtoms::_MOTIF_WM_HINTS, 32, PropModeReplace, (unsigned char*) &hints, 5);

					customFrameMode = false;
				}

				bool XlibWindow::IsCustomFrameModeEnabled()
				{
					return customFrameMode;
				}

				XlibWindow::WindowSizeState XlibWindow::GetSizeState()
				{
					//TODO
					return XlibWindow::WindowSizeState::Restored;
				}

				void XlibWindow::ShowDeactivated()
				{
					//TODO
					Show();
				}

				void XlibWindow::ShowRestored()
				{
					//TODO
					Show();
				}

				void XlibWindow::ShowMaximized()
				{
					//TODO
					Show();
				}

				void XlibWindow::ShowMinimized()
				{
					//TODO
					Show();
				}

				bool XlibWindow::IsVisible()
				{
					return visible;
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
					return IsCustomFrameModeEnabled();
				}

				void XlibWindow::SetTitleBar(bool visible)
				{
					//TODO
					visible ? DisableCustomFrameMode() : EnableCustomFrameMode();
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
					FOREACH(INativeWindowListener*, i, listeners)
					{
						i->Paint();
					}
				}
			}
		}
	}
}

