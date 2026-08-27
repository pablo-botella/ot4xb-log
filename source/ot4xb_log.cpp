#include <windows.h>
#include <shlwapi.h>
#include <richedit.h>
#include <math.h>

#define OT4XB_LOG_WND_CLASS  "11CBDBE2_0AF0_4713_B463_269FA6E2654B"
#define OT4XB_LOG_WND_TITLE  "ot4xb - log"
// -----------------------------------------------------------------------------------------------------------------
#define OT4XB_LOG_MSG_ACTIVATE   (WM_APP + 1 ) 
#define OT4XB_LOG_SHELL_NOTIFY   (WM_APP + 2 ) 
#define OT4XB_LOG_MSG_DESTROY    (WM_APP + 3 ) 
// -----------------------------------------------------------------------------------------------------------------
class app_t;
// -----------------------------------------------------------------------------------------------------------------
static app_t*  p_app_obj = 0;
// -----------------------------------------------------------------------------------------------------------------
static LRESULT __stdcall _main_wnd_proc_( HWND hWnd , UINT nMsg , WPARAM wp , LPARAM lp);
// -----------------------------------------------------------------------------------------------------------------
#define MYHEAPFLAGS    (HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY)
#define _mk_ptr_( cast, ptr, addValue ) ((cast)((void*)( (DWORD)(ptr)+(DWORD)(addValue))))
#define _pgrab(n) (HeapAlloc( GetProcessHeap() , MYHEAPFLAGS , n ))
#define _pfree(p) (HeapFree(GetProcessHeap(),0,p))
// -----------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------
void* _xgrab(UINT n){  void* p = malloc(n); if(p){ ZeroMemory(p,n); } return p;}
void _xfree(void* p){if( p ){ free(p);} }
// -----------------------------------------------------------------------------------------------------------------
// Command line arguments arrive as UTF-16 (CommandLineToArgvW); the program is
// ANSI, so every value it keeps is converted here. An empty value yields 0, so
// the caller falls back to its default.
LPSTR _wide_to_ansi_(LPCWSTR pWide)
{
   if( !pWide || !*pWide ){ return 0; }
   int cb = WideCharToMultiByte(CP_ACP,0,pWide,-1,0,0,0,0);
   if( cb <= 0 ){ return 0; }
   LPSTR p = (LPSTR) _xgrab( (UINT) cb + 1 );
   if( !p ){ return 0; }
   if( WideCharToMultiByte(CP_ACP,0,pWide,-1,p,cb,0,0) <= 0 ){ _xfree(p); return 0; }
   if( !*p ){ _xfree(p); return 0; }
   return p;
}
// -----------------------------------------------------------------------------------------------------------------
BOOL bWriteLogLine(LPSTR pFileName,LPBYTE pBuffer, DWORD cb )
{
   if(! (pFileName && pBuffer) ){ return FALSE;}
   BOOL bResult = FALSE;
   HANDLE hFile = CreateFile(pFileName,GENERIC_WRITE|GENERIC_READ,0,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);

   if( hFile != INVALID_HANDLE_VALUE )
   {
      DWORD dw = 0;
      BYTE  ch = 0;
      SetFilePointer( hFile , 0 , NULL , FILE_END);
      if( cb == (DWORD) -1 ) cb = lstrlen((LPSTR) pBuffer);
      bResult = WriteFile(hFile, pBuffer , cb , &dw , NULL);
      if( bResult){ bResult = WriteFile(hFile, &ch , 1 , &dw , NULL);}
      CloseHandle( hFile );
   }
   return bResult;
}
// -----------------------------------------------------------------------------------------------------------------
LPBYTE pMemoRead(LPSTR pFileName, DWORD* pcb )
{
   DWORD dw,dwr;
   HANDLE hFile = CreateFile(pFileName , GENERIC_READ , 0 , NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
   LPBYTE p;
   if( hFile == INVALID_HANDLE_VALUE ) return 0;
   dw = SetFilePointer( hFile , 0 , 0, FILE_END);
   SetFilePointer( hFile , 0 , 0, FILE_BEGIN );
   p = (LPBYTE) _xgrab(dw + 2);
   BOOL bReadResult = ReadFile(hFile, p, dw, &dwr, 0);
   CloseHandle( hFile );
   if( !bReadResult ) {
      _xfree(p);
      if( pcb ) *pcb = 0;
      return 0;
   }
   if( pcb ) *pcb = dwr;
   return p;
}
// -----------------------------------------------------------------------------------------------------------------
HFONT _create_font_(HDC hDC, LPSTR szFaceName, int iDeciPtHeight,int iDeciPtWidth, int iAttributes, BOOL fLogRes)
{
   FLOAT      cxDpi, cyDpi;
   HFONT      hFont;
   LOGFONT    lf;
   POINT      pt;
   TEXTMETRIC tm;
   
   SaveDC (hDC) ;
   SetGraphicsMode (hDC, GM_ADVANCED) ;
   ModifyWorldTransform (hDC, NULL, MWT_IDENTITY) ;
   SetViewportOrgEx (hDC, 0, 0, NULL) ;
   SetWindowOrgEx   (hDC, 0, 0, NULL) ;
   if (fLogRes)
   {
      cxDpi = (FLOAT) GetDeviceCaps (hDC, LOGPIXELSX) ;
      cyDpi = (FLOAT) GetDeviceCaps (hDC, LOGPIXELSY) ;
   }
   else
   {
      cxDpi = (FLOAT) (25.4 * GetDeviceCaps (hDC, HORZRES) / GetDeviceCaps(hDC, HORZSIZE)) ;
      cyDpi = (FLOAT) (25.4 * GetDeviceCaps (hDC, VERTRES) / GetDeviceCaps(hDC, VERTSIZE)) ;
   }
   pt.x = (int) (iDeciPtWidth  * cxDpi / 72) ;
   pt.y = (int) (iDeciPtHeight * cyDpi / 72) ;
   DPtoLP (hDC, &pt, 1) ;
   lf.lfHeight         = - (int) (fabs((FLOAT)pt.y) / 10.0 + 0.5) ;
   lf.lfWidth          = 0 ;
   lf.lfEscapement     = 0 ;
   lf.lfOrientation    = 0 ;
   lf.lfWeight         = (LONG) (iAttributes & 1 ? 700 : 0 ); // Bold
   lf.lfItalic         = (BYTE) (iAttributes & 2 ?   1 : 0 );
   lf.lfUnderline      = (BYTE) (iAttributes & 4 ?   1 : 0 );
   lf.lfStrikeOut      = (BYTE) (iAttributes & 8 ?   1 : 0 );
   lf.lfCharSet        = 0 ;
   lf.lfOutPrecision   = 0 ;
   lf.lfClipPrecision  = 0 ;
   lf.lfQuality        = 0 ;
   lf.lfPitchAndFamily = 0 ;
   
   lstrcpy (lf.lfFaceName, szFaceName) ;
   
   hFont = CreateFontIndirect (&lf) ;
   
   if (iDeciPtWidth != 0)
   {
      hFont = (HFONT) SelectObject (hDC, hFont) ;
      GetTextMetrics (hDC, &tm) ;
      DeleteObject (SelectObject (hDC, hFont)) ;
      lf.lfWidth = (int) (tm.tmAveCharWidth * fabs((FLOAT)pt.x) / fabs((FLOAT)pt.y) + 0.5) ;
      hFont = CreateFontIndirect (&lf) ;
   }
   RestoreDC (hDC, -1) ;
   return hFont ;
}
// -----------------------------------------------------------------------------------------------------------------
class app_t
{
   public:
      HINSTANCE m_hInstance;
      HWND m_hWnd;
      HWND m_hRtf;      
      BOOL  m_bInSizeLoop;
      HFONT m_hFont;
      HICON m_hIcon;
      BOOL  m_bShutDownInit;      
      BOOL  m_bPopup;    
      LPSTR m_pLogName;
      LPSTR m_szClsWnd;
      LPSTR m_szWndName;
      LPSTR m_szIconSpec;
      BOOL  m_bOwnIcon;
      ATOM  m_hClsHandle;
      // ---------------------------------------------------------------------------------
      app_t( HINSTANCE hInstance )
      {
         p_app_obj = this;
         m_hInstance = hInstance;
         // ----------------------
         init_data_members();
         // ----------------------
         LPSTR pLogName = 0;
         parse_command_line( &pLogName );
         apply_default_params();
         // -------------------------
         if( activate_running_instance() ){ _xfree( (void*) pLogName ); return; }
         // -------------------------
         if( ! register_window_class() ){ _xfree( (void*) pLogName ); return; }
         // ------
         LoadLibrary( "Riched20.dll");
         // ------
         build_log_file_name( pLogName );
         // ------
         create_default_font();
      }
      // ---------------------------------------------------------------------------------
      void init_data_members(void)
      {
         m_bPopup         = 0;
         m_bShutDownInit  = 0;
         m_hWnd           = 0;
         m_hRtf           = 0;
         m_bInSizeLoop    = FALSE;
         m_hFont          = 0;
         m_hIcon          = 0;
         m_pLogName       = 0;
         m_szClsWnd       = 0;
         m_szWndName      = 0;
         m_szIconSpec     = 0;
         m_bOwnIcon       = FALSE;
         m_hClsHandle     = 0;
      }
      // ---------------------------------------------------------------------------------
      //   ot4xb_log.exe [--class <name>] [--title <text>] [--log <name>]
      //                 [--icon <file.ico>]
      //
      //   Values may be quoted ("Some title"): the shell parser unquotes them.
      //   Options are case insensitive, may appear in any order, and an unknown
      //   option - or one with no value - is ignored. A missing option keeps
      //   its default.
      //
      //   The class name and the title land in their members; the log name is
      //   handed back to the caller, which resolves it into a full path later.
      // ---------------------------------------------------------------------------------
      void parse_command_line( LPSTR* ppLogName )
      {
         int     nArgs = 0;
         LPWSTR* pArgv = CommandLineToArgvW( GetCommandLineW() , &nArgs );
         if( ! pArgv ){ return; }
         for( int i = 1 ; i < nArgs ; i++ )
         {
            LPSTR* ppDst = get_option_target( pArgv[i] , ppLogName );
            if( ! ppDst ){ continue; }
            if( (i + 1) >= nArgs ){ break; }
            i++;
            if( *ppDst ){ _xfree( (void*) *ppDst ); *ppDst = 0; }
            *ppDst = _wide_to_ansi_( pArgv[i] );
         }
         LocalFree( (HLOCAL) pArgv );
      }
      // ---------------------------------------------------------------------------------
      // Where the value of an option is stored; 0 when the name is not an option.
      LPSTR* get_option_target( LPCWSTR pOption , LPSTR* ppLogName )
      {
         if( lstrcmpiW( pOption , L"--class" ) == 0 ){ return &m_szClsWnd;   }
         if( lstrcmpiW( pOption , L"--title" ) == 0 ){ return &m_szWndName;  }
         if( lstrcmpiW( pOption , L"--log"   ) == 0 ){ return ppLogName;     }
         if( lstrcmpiW( pOption , L"--icon"  ) == 0 ){ return &m_szIconSpec; }
         return 0;
      }
      // ---------------------------------------------------------------------------------
      void apply_default_params(void)
      {
         if( ! m_szClsWnd )
         {
            m_szClsWnd = (LPSTR) _xgrab(sizeof(OT4XB_LOG_WND_CLASS)+1);
            lstrcpy( m_szClsWnd,OT4XB_LOG_WND_CLASS);
         }
         // -------------------------
         if( ! m_szWndName )
         {
            m_szWndName = (LPSTR) _xgrab(sizeof(OT4XB_LOG_WND_TITLE)+1);
            lstrcpy( m_szWndName,OT4XB_LOG_WND_TITLE );
         }
      }
      // ---------------------------------------------------------------------------------
      // One viewer per window class: hand the log over to the instance already
      // running and tell the caller to give up.
      BOOL activate_running_instance(void)
      {
         HWND h = FindWindow(m_szClsWnd,0);
         if( h && IsWindow(h) )
         {
            PostMessage(h,OT4XB_LOG_MSG_ACTIVATE, 0 , 0);
            return TRUE;
         }
         return FALSE;
      }
      // ---------------------------------------------------------------------------------
      // The icon given with --icon, or the one built into this executable.
      // A custom icon is ours to destroy; the embedded one is shared and must
      // not be.
      void load_icon(void)
      {
         m_hIcon = load_custom_icon();
         if( m_hIcon ){ m_bOwnIcon = TRUE; return; }
         m_hIcon = (HICON) LoadImage(m_hInstance,MAKEINTRESOURCE(1),IMAGE_ICON,0,0,LR_DEFAULTSIZE|LR_VGACOLOR|LR_SHARED);
         m_bOwnIcon = FALSE;
      }
      // ---------------------------------------------------------------------------------
      //   --icon <file.ico>
      //
      //   An .ico file read from disk. Anything that fails to load - a missing
      //   file, a file that is not an icon - leaves the built-in icon in place,
      //   silently.
      // ---------------------------------------------------------------------------------
      HICON load_custom_icon(void)
      {
         if( !( m_szIconSpec && *m_szIconSpec ) ){ return 0; }
         return (HICON) LoadImage(0,m_szIconSpec,IMAGE_ICON,0,0,LR_LOADFROMFILE|LR_DEFAULTSIZE|LR_VGACOLOR);
      }
      // ---------------------------------------------------------------------------------
      BOOL register_window_class(void)
      {
         load_icon();
         // ------
         WNDCLASSEX wc;
         ZeroMemory(&wc,sizeof(WNDCLASSEX));
         wc.cbSize = sizeof(wc);
         wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
         wc.hInstance = m_hInstance;
         wc.hCursor   =  LoadCursor(0,IDC_ARROW);
         wc.lpszClassName = m_szClsWnd;
         wc.lpfnWndProc = _main_wnd_proc_;
         wc.hIcon = m_hIcon;
         m_hClsHandle = RegisterClassEx(&wc);
         return ( m_hClsHandle != 0 );
      }
      // ---------------------------------------------------------------------------------
      // The log always sits next to the executable: only the name given with
      // --log is used, its path and extension stripped. Frees pLogName.
      void build_log_file_name( LPSTR pLogName )
      {
         m_pLogName  = (LPSTR) _xgrab(1024);
         GetModuleFileName( NULL , m_pLogName , 1023 );
         if( pLogName )
         {
            PathStripPath(pLogName);
            PathRemoveExtension(pLogName);
            if( *pLogName )
            {
               PathRemoveFileSpec(m_pLogName);
               PathAppend(m_pLogName,pLogName);
            }
            _xfree( pLogName ); pLogName = 0;
         }
         PathRenameExtension(m_pLogName,".log");
      }
      // ---------------------------------------------------------------------------------
      void create_default_font(void)
      {
         HDC hDC = GetDC(0);
         m_hFont = _create_font_(hDC,"Verdana",80,0,0,1);
         ReleaseDC(0,hDC);
      }
      // ---------------------------------------------------------------------------------      
      ~app_t(void)
      {
         if( m_hFont ){ DeleteObject( (HGDIOBJ) m_hFont); m_hFont = 0; }
         if( m_hIcon && m_bOwnIcon ){ DestroyIcon( m_hIcon ); m_hIcon = 0; m_bOwnIcon = FALSE; }
      };
      // ---------------------------------------------------------------------------------      
      LRESULT wndproc_main( HWND hWnd , UINT nMsg , WPARAM wp , LPARAM lp)
      {
         switch(nMsg)
         {
            case OT4XB_LOG_MSG_DESTROY:
            {
               if( ((DWORD) wp == (DWORD) lp) && ((DWORD) lp == (DWORD) this ) )
               {
                  m_bShutDownInit = TRUE;
                  DestroyWindow( m_hWnd );
               }
               return 0;
            }
            case WM_CLOSE:
            {
               ShowWindow(hWnd,SW_HIDE);
               return 0;
            }
            case OT4XB_LOG_SHELL_NOTIFY:
            {
               switch(LOWORD(lp))
               {
                   case WM_LBUTTONDOWN:
                   {
                      if( IsWindowVisible(hWnd) )
                      {
                         ShowWindow(hWnd,SW_HIDE);
                      }
                      else
                      {
                         PostMessage(hWnd,OT4XB_LOG_MSG_ACTIVATE,0,0);
                      }
                      return 0;
                   }                   
                   case WM_CONTEXTMENU: { return OnContextMenu(); }
                   
                }
                return 0;
            }
            case OT4XB_LOG_MSG_ACTIVATE:
            {
               ShowWindow(hWnd,SW_SHOW);
               if(IsIconic(hWnd)){ ShowWindow(hWnd,SW_RESTORE); }               
               SetForegroundWindow( hWnd);                              
               BringWindowToTop(hWnd);               
               return 0;
            }
            case WM_CREATE:
            {
               RECT rc;
               GetClientRect(hWnd,&rc);
               m_hRtf = CreateWindowEx(WS_EX_CLIENTEDGE,
                                       "RichEdit20A","",
                                       WS_VISIBLE|WS_VSCROLL|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_READONLY|WS_TABSTOP|WS_CHILD,
                                       0,0,rc.right,rc.bottom,hWnd,(HMENU) 1,m_hInstance,0);
               if( m_hRtf )
               {
                  SendMessage(m_hRtf, WM_SETFONT , (WPARAM) m_hFont,1);
                  SendMessage( m_hRtf,EM_SETBKGNDCOLOR,0,0xF0F0F0);
                  SendMessage(m_hRtf,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS );
               }
               
               NOTIFYICONDATA ni;
               ZeroMemory(&ni,sizeof(ni));
               ni.cbSize = NOTIFYICONDATA_V2_SIZE;
               ni.hWnd   = hWnd;
               ni.hIcon  = m_hIcon;
               ni.uID    = 1;
               lstrcpy(ni.szTip,m_szWndName);
               ni.uCallbackMessage = OT4XB_LOG_SHELL_NOTIFY;
               ni.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
               Shell_NotifyIcon(0,&ni);
               ni.uFlags = 0;
               ni.uVersion = NOTIFYICON_VERSION;                
               Shell_NotifyIcon(NIM_SETVERSION,&ni);               
               
               return DefWindowProcA(hWnd,nMsg,wp,lp);                                             
            }
            case WM_ENTERSIZEMOVE:
            {
               m_bInSizeLoop = TRUE;
               break;
            }
            case WM_EXITSIZEMOVE:
            {
               m_bInSizeLoop = FALSE;
               adjust_rects();
               break;
            }
            case WM_SIZE:
            {
               if( (wp != SIZE_MINIMIZED) && (!m_bInSizeLoop) ){ adjust_rects(); }
               return 0;
            }            
            case WM_NCDESTROY:
            {
               NOTIFYICONDATA ni;
               ZeroMemory(&ni,sizeof(ni));
               ni.cbSize = NOTIFYICONDATA_V2_SIZE;
               ni.hWnd   = hWnd;
               ni.uID    = 1;
               Shell_NotifyIcon(NIM_DELETE,&ni);
                           
               PostQuitMessage(0);
               return DefWindowProcA(hWnd,nMsg,wp,lp);                  
            }
            case WM_COPYDATA:
            {
               if( !lp ){ return 0;}            
               COPYDATASTRUCT* pcds = reinterpret_cast<COPYDATASTRUCT*>(lp);
               if( pcds->lpData && pcds->cbData )
               {
                  UINT cb = pcds->cbData;
                  void* p = _xgrab( cb + 4 );                  
                  memcpy(p,pcds->lpData,cb);
                  _mk_ptr_(LPBYTE,p,cb)[0] = 13;
                  _mk_ptr_(LPBYTE,p,cb)[1] = 10;
                  ReplyMessage(1);
                  bWriteLogLine(m_pLogName,(LPBYTE) p,cb+2);
                  _mk_ptr_(LPBYTE,p,cb)[0] = 0;                  
                  ShowLogLine( (LPSTR) p , cb);
                  _xfree(p);
               }
               return 0;
            }
            case WM_DESTROY:
            {
               m_bShutDownInit = TRUE;
               break;
            
            }
            case WM_NOTIFY:
            {
               NMHDR* ph = reinterpret_cast<NMHDR*>(lp);            
               MSGFILTER* mf = reinterpret_cast<MSGFILTER*>(lp);
                              
               if( lp && !m_bShutDownInit && ph->hwndFrom == m_hRtf && ph->code == EN_MSGFILTER )
               {
                  if( mf->msg == WM_RBUTTONUP ){ OnContextMenu(); return 0; }
               }
               break;
            }
         }
         return DefWindowProcA(hWnd,nMsg,wp,lp);      
      };
      // ---------------------------------------------------------------------------------            
      LRESULT OnContextMenu( void )
      {
         HMENU hPopup = CreatePopupMenu();
         POINT pt;
         char sz[1024];
         AppendMenu(hPopup, MF_STRING | ( m_bPopup ? MF_CHECKED : 0) , 0x405 , "Popup on Event");
         AppendMenu(hPopup, MF_SEPARATOR,0,0);                               
         AppendMenu(hPopup, MF_STRING , 0x404 , ( IsWindowVisible(m_hWnd ) ? "Hide Window" : "Show Window" )); 
         AppendMenu(hPopup, MF_SEPARATOR,0,0);                      
         AppendMenu(hPopup, MF_STRING , 0x403 , "Load History");                                            
         AppendMenu(hPopup, MF_STRING , 0x402 , "Clear Log Window");                                                                  
         AppendMenu(hPopup, MF_SEPARATOR,0,0);
         wsprintf(sz,"E&xit %s" , m_szWndName );
         AppendMenu(hPopup, MF_STRING , 0x401 , sz);
         GetCursorPos(&pt);
         SetForegroundWindow(m_hWnd);
         int i = TrackPopupMenuEx(hPopup,TPM_RIGHTALIGN|TPM_RETURNCMD|TPM_NONOTIFY|TPM_BOTTOMALIGN,pt.x,pt.y,m_hWnd,0);
         DestroyMenu(hPopup);
         switch( i )
         {
            case 0x401 : { PostMessage(m_hWnd,OT4XB_LOG_MSG_DESTROY,(WPARAM) this,(LPARAM) this); return 0; }
            case 0x402 : 
            {
               SendMessage( m_hRtf,WM_SETTEXT,0,0);
               return 0;
            }
            case 0x403 : 
            {
               LoadHistory();
               return 0;
            }   
            case 0x404: 
            {
               if( IsWindowVisible(m_hWnd) ){ShowWindow(m_hWnd,SW_HIDE);}
               else { PostMessage(m_hWnd,OT4XB_LOG_MSG_ACTIVATE,0,0); }
               return 0;
            }
            case 0x405: { m_bPopup = !m_bPopup; return 0; }             
         }
         return 0;
      }
      // ---------------------------------------------------------------------------------
            
      void adjust_rects(void)
      {
         RECT rc;
         GetClientRect( m_hWnd , &rc );
         SetWindowPos(m_hRtf,0,0,0,rc.right,rc.bottom,SWP_NOOWNERZORDER);
      }
      // ---------------------------------------------------------------------------------                  
      void LoadHistory( void )
      {
         if( !m_hRtf ){ return; }
         SendMessage( m_hRtf,WM_SETTEXT,0,0);      
         DWORD cb = 0;
         LPBYTE pp = pMemoRead(m_pLogName,&cb);
         LPBYTE p  = pp;
         if( ! p ){ return; }
         BOOL bPop = m_bPopup;
         m_bPopup = FALSE;
         while( cb )
         {
            DWORD n = 0;
            while((n < cb) && p[n] ){ n++; }
            if( cb <= n ){ goto TheEnd; }
            if( p[n]){ goto TheEnd; }            
            if( n ) ShowLogLine( (LPSTR) p , n );
            n++; cb -= n; p = _mk_ptr_(LPBYTE,p,n);
         }
         TheEnd: ;
         m_bPopup = bPop;         
         _xfree( (void*) pp );
      }
      // ---------------------------------------------------------------------------------
            
      void ShowLogLine( LPSTR p,UINT cb)
      {
         if( m_hRtf && p && cb )
         {
            CHARFORMAT cf;
            ZeroMemory(&cf,sizeof(cf));
            cf.cbSize  = sizeof(cf);
            cf.dwMask  = CFM_COLOR | CFM_CHARSET;// | CFM_SIZE | CFM_FACE;
            cf.yHeight = 150;
            cf.crTextColor = 255;
            cf.bCharSet = ANSI_CHARSET;
            strcpy_s(cf.szFaceName,LF_FACESIZE,"Verdana");
            
            LPSTR ps = strstr(p,"\r\n");
            if( ps == p ){ ps = 0; }
            if(ps){ps[0] = 0;}
         
            SendMessage(m_hRtf,EM_SETSEL,(WPARAM) -1,(LPARAM) -1);
            SendMessage(m_hRtf,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM) &cf);
            SendMessage(m_hRtf,EM_REPLACESEL,FALSE,(LPARAM) p);         
            if( ps )
            {
               ps[0] = 13;
               cf.crTextColor = 0;            
               SendMessage(m_hRtf,EM_SETSEL,(WPARAM) -1,(LPARAM) -1);
               SendMessage(m_hRtf,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM) &cf);
               SendMessage(m_hRtf,EM_REPLACESEL,FALSE,(LPARAM) ps);               
            }
            cf.crTextColor = 0xFF0000;   
            SendMessage(m_hRtf,EM_SETSEL,(WPARAM) -1,(LPARAM) -1);            
            SendMessage(m_hRtf,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM) &cf);            
            SendMessage(m_hRtf,EM_REPLACESEL,FALSE,(LPARAM)
             "________________________________________________________________________________"
             "________________________________________________________________________________"             
             "________________________________________________________________________________"             
             "________________________________________________________________________________"             
             "________________________________________________________________________________"             
             "\r\n");
             
                        
            SendMessage(m_hRtf,EM_SCROLLCARET,0,0);                                                
            SendMessage(m_hRtf,WM_VSCROLL,SB_BOTTOM,0);    
            if( m_bPopup && !IsWindowVisible(m_hWnd) )
            { 
               ShowWindow(m_hWnd,SW_SHOWNOACTIVATE);
            }                                            
         }
      }
      
      // ---------------------------------------------------------------------------------                  
      void AdjustToMonitor( int & x , int & y , int & cx , int & cy )
      {
         HMONITOR hMonitor = MonitorFromWindow(0,MONITOR_DEFAULTTOPRIMARY);      
         MONITORINFO mi;         
         ZeroMemory( &mi, sizeof(MONITORINFO));
         mi.cbSize = sizeof(MONITORINFO);
         if(!GetMonitorInfo(hMonitor,&mi))
         {
            RECT rc;
            GetClientRect(GetDesktopWindow(),&rc);
            x = y = 0; cx = rc.right; cy = rc.bottom - 60;
            return;
         }
         x = mi.rcWork.left;
         y = mi.rcWork.top;
         cx = mi.rcWork.right  - x;   if( cx > 540){cx = 540;}
         cy = mi.rcWork.bottom - y;
      };
      // ---------------------------------------------------------------------------------                  
      int run(void)
      {
         int x,y,cx,cy;
         AdjustToMonitor(x,y,cx,cy);
         m_hWnd = CreateWindowEx( WS_EX_TOOLWINDOW  ,
                                  m_szClsWnd,m_szWndName,
                                  WS_OVERLAPPEDWINDOW  ,
                                  x,y,cx,cy,0,0,m_hInstance,0);
         if( !m_hWnd ){return 0;}
         
         MSG msg;
         while( GetMessage(&msg,0,0,0) )
         {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
         return 1;
      };
      
      // ---------------------------------------------------------------------------------            
};
// -----------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------


int WINAPI WinMain(  HINSTANCE hInstance, HINSTANCE ,LPSTR, int)
{
   app_t app(hInstance);
   if( app.m_hClsHandle ){ app.run(); }
   return 0;
}
// -----------------------------------------------------------------------------------------------------------------
static LRESULT __stdcall _main_wnd_proc_( HWND hWnd , UINT nMsg , WPARAM wp , LPARAM lp)
{
   if( p_app_obj ) return p_app_obj->wndproc_main(hWnd,nMsg,wp,lp);
   return DefWindowProcA(hWnd,nMsg,wp,lp);
}
// -----------------------------------------------------------------------------------------------------------------


