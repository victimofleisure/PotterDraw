// Copyleft 2017 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      12mar17	initial version
		
*/

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "PotterDraw.h"
#include "PotterDrawDoc.h"
#include "PotterDrawView.h"
#include "MainFrm.h"
#include "PathStr.h"
#include "UndoCodes.h"
#include "OptionsDlg.h"
#include "RecordStatusDlg.h"
#include "DocIter.h"
#include "FocusEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_RESOLUTION,
	ID_INDICATOR_ZOOM,
	ID_INDICATOR_FRAME_RATE,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

#define RK_MDI_TABS	_T("MDITabs")

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);
	m_pActiveView = NULL;
	m_pRecordStatusDlg = NULL;
	m_bMDITabs = theApp.GetInt(RK_MDI_TABS, TRUE) != 0;
	m_bPreFullScreenWasZoomed = false;
	m_bDeferredUpdate = false;
	m_bDeferredSizing = false;
}

CMainFrame::~CMainFrame()
{
	theApp.WriteInt(RK_MDI_TABS, m_bMDITabs);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	theApp.m_pMainWnd = this;
	if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;
	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	m_mdiTabParams.m_style = CMFCTabCtrl::STYLE_3D_ONENOTE; // other styles available...
	m_mdiTabParams.m_bActiveTabCloseButton = TRUE;      // set to FALSE to place close button at right of tab area
	m_mdiTabParams.m_bTabIcons = FALSE;    // set to TRUE to enable document icons on MDI taba
	m_mdiTabParams.m_bAutoColor = TRUE;    // set to FALSE to disable auto-coloring of MDI tabs
	m_mdiTabParams.m_bDocumentMenu = TRUE; // enable the document menu at the right edge of the tab area
	EnableMDITabbedGroups(m_bMDITabs, m_mdiTabParams);

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
#if 0	// ck: disable user-defined toolbars for now
	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);
#endif

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Load menu item image (not placed on any standard toolbars):
//	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES, theApp.m_bHiColorIcons ? IDB_MENU_IMAGES_24 : 0);

	// create docking windows
	if (!CreateDockingWindows())
	{
		TRACE0("Failed to create docking windows\n");
		return -1;
	}
	m_wndPropertiesBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndPropertiesBar);
	m_wndModulationBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndModulationBar);
	m_wndPaletteBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndPaletteBar);
	m_wndOscilloscopeBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndOscilloscopeBar);
	m_wndSplineBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndSplineBar);
	UpdateOptions();

	// Enable enhanced windows management dialog
	EnableWindowsDialog(ID_WINDOW_MANAGER, ID_WINDOW_MANAGER, TRUE);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

#if 0	// ck: disable toolbar button customization for now
	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}
#endif

#if 0	// ck: disable menu personalization
	// enable menu personalization (most-recently used commands)
	// TODO: define your own basic commands, ensuring that each pulldown menu has at least one basic command.
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_PRINT);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);
	lstBasicCommands.AddTail(ID_SORTING_SORTALPHABETIC);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYTYPE);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYACCESS);
	lstBasicCommands.AddTail(ID_SORTING_GROUPBYTYPE);

	CMFCToolBar::SetBasicCommands(lstBasicCommands);
#endif

	// Switch the order of document name and application name on the window title bar. This
	// improves the usability of the taskbar because the document name is visible with the thumbnail.
	ModifyStyle(0, FWS_PREFIXTITLE);

	SetTimer(FRAME_RATE_TIMER, FRAME_RATE_TIMER_PERIOD, NULL);
	EnableFullScreenMode(ID_WINDOW_FULL_SCREEN);
	EnableFullScreenMainMenu(false);

//	theApp.GetContextMenuManager()->AddMenu(_T("View"), IDR_POPUP_EDIT);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

BOOL CMainFrame::CreateDockingWindows()
{
	// create properties bar
	CString sTitle;
	sTitle.LoadString(IDS_PROPERTIES_BAR);
	DWORD	dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI;
	CPotProperties	props;
	m_wndPropertiesBar.SetInitialProperties(props);
	if (!m_wndPropertiesBar.Create(sTitle, this, CRect(0, 0, 300, 200), TRUE, IDS_PROPERTIES_BAR, dwStyle))
	{
		TRACE0("Failed to create properties bar\n");
		return FALSE; // failed to create
	}
	// create modulation bar
	sTitle.LoadString(IDS_MODULATION_BAR);
	dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI;
	CModulationProps	ModProps;
	ModProps.SetDefault();
	m_wndModulationBar.SetInitialProperties(ModProps);
	if (!m_wndModulationBar.Create(sTitle, this, CRect(0, 0, 300, 200), TRUE, IDS_MODULATION_BAR, dwStyle))
	{
		TRACE0("Failed to create modulation bar\n");
		return FALSE; // failed to create
	}
	// create palette bar
	sTitle.LoadString(IDS_PALETTE_BAR);
	dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI;
	if (!m_wndPaletteBar.Create(sTitle, this, CRect(0, 0, 450, 200), TRUE, IDS_PALETTE_BAR, dwStyle))
	{
		TRACE0("Failed to create palette bar\n");
		return FALSE; // failed to create
	}
	// create oscilloscope bar
	sTitle.LoadString(IDS_OSCILLOSCOPE_BAR);
	dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI;
	if (!m_wndOscilloscopeBar.Create(sTitle, this, CRect(0, 0, 300, 200), TRUE, IDS_OSCILLOSCOPE_BAR, dwStyle))
	{
		TRACE0("Failed to create oscilloscope bar\n");
		return FALSE; // failed to create
	}
	// create spline bar
	sTitle.LoadString(IDS_SPLINE_BAR);
	dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI;
	if (!m_wndSplineBar.Create(sTitle, this, CRect(0, 0, 300, 200), TRUE, IDS_SPLINE_BAR, dwStyle))
	{
		TRACE0("Failed to create spline bar\n");
		return FALSE; // failed to create
	}
	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
//	HICON hPropertiesBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
//	m_wndPropertiesBar.SetIcon(hPropertiesBarIcon, FALSE);

	UpdateMDITabbedBarsIcons();
}

void CMainFrame::OnActivateView(CView *pView)
{
	// dynamic cast because other view types are possible, e.g. print preview
	CPotterDrawView	*pActiveView = DYNAMIC_DOWNCAST(CPotterDrawView, pView);
	if (pActiveView != m_pActiveView) {
		m_pActiveView = pActiveView;
		if (!m_bDeferredUpdate)
			OnUpdate(NULL);
		bool	bNewEnable = pActiveView != NULL;
		bool	bOldEnable = m_wndPropertiesBar.GetWindow(GW_CHILD)->IsWindowEnabled() != 0;
		if (bNewEnable != bOldEnable) {	// if first or last view
			EnableChildWindows(m_wndPropertiesBar, bNewEnable);
			EnableChildWindows(m_wndPaletteBar, bNewEnable);
			EnableChildWindows(m_wndModulationBar, bNewEnable);
			EnableChildWindows(m_wndSplineBar, bNewEnable);
		}
	}
}

void CMainFrame::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
//	printf("CMainFrame::OnUpdate pSender=%Ix lHint=%Id pHint=%Ix\n", pSender, lHint, pHint);
	CPotterDrawDoc	*pDoc = GetActiveMDIDoc();
	if (pDoc != NULL) {
		if (pSender != reinterpret_cast<CView *>(&m_wndPropertiesBar)) {	// if sender isn't properties bar
			m_wndPropertiesBar.SetProperties(*pDoc);	// update properties bar
		}
		if (pSender != reinterpret_cast<CView *>(&m_wndPaletteBar)) {	// if sender isn't palette bar
			m_wndPaletteBar.SetPalette(pDoc->m_arrPalette, pDoc->m_iPaletteCurSel);	// update palette bar
		}
		if (pSender != reinterpret_cast<CView *>(&m_wndModulationBar)) {	// if sender isn't modulation bar
			UpdateModulationBar(pDoc);
		}
		if (pSender != reinterpret_cast<CView *>(&m_wndSplineBar)) {	// if sender isn't spline bar
			m_wndSplineBar.m_wndSpline.SetState(pDoc->m_arrSpline);
		}
	} else {	// no active document
		CPotProperties	props;
		m_wndPropertiesBar.SetProperties(props);
		m_wndPropertiesBar.SetCurSel(-1);
		m_wndModulationBar.SetProperties(props.m_Mod[0]);
		m_wndModulationBar.SetCurSel(-1);
		CArray<COLORREF, COLORREF> arrColor;
		m_wndPaletteBar.SetPalette(arrColor);
		m_wndOscilloscopeBar.Update();
		CSplineState	arrSpline;
		m_wndSplineBar.m_wndSpline.SetState(arrSpline);
	}
}

CPotterDrawDoc *CMainFrame::GetActiveMDIDoc()
{
	if (m_pActiveView != NULL)
		return(m_pActiveView->GetDocument());
	return(NULL);
}

void CMainFrame::UpdateToolbar()
{
	m_wndToolBar.OnUpdateCmdUI(this, false);
}

CString	CMainFrame::GetResolutionString() const
{
	CString	sRes;
	if (m_pActiveView != NULL) {
		CSize	sz(m_pActiveView->GetClientSize());
		sRes.Format(_T("%d x %d"), sz.cx, sz.cy);
	}
	return sRes;
}

void CMainFrame::FullScreen(bool bEnable)
{
	if (bEnable == (IsFullScreen() != 0))	// if already in requested state
		return;	// nothing to do
	m_bDeferredSizing = true;	// prevent views from prematurely resizing mesh
	SetRedraw(false);	// disable painting to reduce flicker
	CMDIChildWnd	*pChildFrm = MDIGetActive();
	if (bEnable) {	// if entering full screen mode
		EnableMDITabbedGroups(FALSE, m_mdiTabParams);	// disable MDI tabs
		if (pChildFrm != NULL) {	// if child frame active
			m_bPreFullScreenWasZoomed = pChildFrm->IsZoomed() != 0;	// save maximized state
			MDIMaximize(pChildFrm);	// maximize child frame
		}
	} else {	// exiting full screen mode
		EnableMDITabbedGroups(m_bMDITabs, m_mdiTabParams);
		UpdateMDITabbedBarsIcons();
		if (!m_bMDITabs && pChildFrm != NULL) {	// if not tabbed MDI and child frame active
			pChildFrm->ModifyStyle(0, WS_BORDER | WS_THICKFRAME);	// restore border
			if (!m_bPreFullScreenWasZoomed)	// if not maximized prior to going full screen
				MDIRestore(pChildFrm);	// restore normal window state
		}
	}
	ShowFullScreen();	// toggle full screen mode
	if (bEnable) {	// if entering full screen mode
		CWnd	*pFullScreenDlg = GetWindow(GW_ENABLEDPOPUP);	// find full screen dialog
		if (pFullScreenDlg != NULL)
			pFullScreenDlg->ShowWindow(SW_HIDE);	// hide full screen dialog
		if (pChildFrm != NULL)
			pChildFrm->ModifyStyle(WS_BORDER | WS_THICKFRAME, 0);	// remove border
	}
	SetRedraw(true);	// reenable painting
	PostMessage(UWM_DEFERRED_SIZING);	// resize meshes after message loop settles down
}

void CMainFrame::ShowRecordStatusDlg(bool bShow)
{
	bool	bShowing = m_pRecordStatusDlg != NULL;
	if (bShow == bShowing)	// if already in requested state
		return;	// nothing to do
	if (bShow) {
		m_pRecordStatusDlg = new CRecordStatusDlg;
		m_pRecordStatusDlg->Create(CRecordStatusDlg::IDD, this);
	} else {
		m_pRecordStatusDlg->SendMessage(WM_CLOSE);
	}
}

void CMainFrame::OnRecord(bool bEnable)
{
	if (!bEnable)	// if stopping
		SendMessageToDescendants(UWM_SET_RECORD, 0);
	ShowRecordStatusDlg(bEnable);
}

void CMainFrame::UpdateOptions()
{
	m_wndPropertiesBar.EnableDescriptionArea(theApp.m_Options.m_bPropertyDescrips);
	m_wndModulationBar.EnableDescriptionArea(theApp.m_Options.m_bPropertyDescrips);
	m_wndOscilloscopeBar.SetShowAllModulations(theApp.m_Options.m_bPlotAllModulations);
	m_wndSplineBar.m_wndSpline.SetZoomStep(theApp.m_Options.m_fSplineZoomStep / 100 + 1);
}

void CMainFrame::UpdateModulationBar(const CPotterDrawDoc *pDoc)
{
	CModulationProps	props;
	CString	sTargetName;
	int	iProp = pDoc->m_iModTarget;
	if (iProp < 0) {	// if document doesn't specify a modulation target
		m_wndModulationBar.GetProperties(props);
		iProp = props.m_iTarget;	// use modulation bar's current target
	}
	if (iProp >= 0) {	// if valid target
		props = pDoc->m_Mod[iProp];
		props.m_iTarget = iProp;
		m_wndModulationBar.SetProperties(props);
	}
	m_wndModulationBar.EnableModulation(CPotProperties::CanModulate(iProp));
	if (m_wndOscilloscopeBar.IsWindowVisible())
		m_wndOscilloscopeBar.Update();
}

void CMainFrame::GotoNextPane(bool bPrev)
{
	CTypedPtrArray<CPtrArray, CWnd*>	arrPane;
	if (m_pActiveView != NULL)	// if active view exists
		arrPane.Add(m_pActiveView);	// add to pane array
	CDockingManager	*pDockMgr = GetDockingManager();
	CObList	lstBar;
	pDockMgr->GetPaneList(lstBar, 0, RUNTIME_CLASS(CDockablePane));
	POSITION	pos = lstBar.GetHeadPosition();
	while (pos != NULL) {
		CDockablePane	*pPane = DYNAMIC_DOWNCAST(CDockablePane, lstBar.GetNext(pos));
		if (pPane != NULL && pPane->IsVisible())	// if dockable pane and visible
			arrPane.Add(pPane);	// add to pane array
	}
	INT_PTR	nPanes = arrPane.GetSize();
	if (!nPanes)	// if no panes
		return;	// nothing to do
	HWND	hFocusWnd = ::GetFocus();	// get focus window handle
	INT_PTR	iFocusWnd = nPanes;	// default to beyond last pane
	for (INT_PTR iPane = 0; iPane < nPanes; iPane++) {	// for each pane
		CWnd	*pPane = arrPane[iPane];
		if (hFocusWnd == pPane->m_hWnd || ::IsChild(pPane->m_hWnd, hFocusWnd)) {
			iFocusWnd = iPane;	// pane or one of its children has focus
			break;	// focus window found so stop iterating
		}
	}
	if (bPrev) {	// if go to previous pane
		iFocusWnd--;	// decrement index
		if (iFocusWnd < 0)	// if before first element
			iFocusWnd = nPanes - 1;	// wrap around
	} else {	// go to next pane
		iFocusWnd++;	// increment index
		if (iFocusWnd >= nPanes)	// if beyond last element
			iFocusWnd = 0;	// wrap around
	}
	CWnd	*pPane = arrPane[iFocusWnd];
	ASSERT(pPane != NULL);	// else logic error above
	pPane->SetFocus();	// focus next/prev pane
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWndEx::Dump(dc);
}
#endif //_DEBUG

// CMainFrame message map

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_WINDOW_MANAGER, OnWindowManager)
	ON_COMMAND(ID_VIEW_CUSTOMIZE, OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_OFF_2003, ID_VIEW_APPLOOK_WIN_XP, OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_OFF_2003, ID_VIEW_APPLOOK_WIN_XP, OnUpdateApplicationLook)
	ON_WM_SETTINGCHANGE()
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_SETUP, OnUpdateFilePrintSetup)
	ON_WM_CLOSE()
	ON_REGISTERED_MESSAGE(AFX_WM_AFTER_TASKBAR_ACTIVATE, OnAfterTaskbarActivate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_ZOOM, OnUpdateIndicatorZoom)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FRAME_RATE, OnUpdateIndicatorFrameRate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_RESOLUTION, OnUpdateIndicatorResolution)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_COMMAND(ID_WINDOW_FULL_SCREEN, OnWindowFullScreen)
	ON_COMMAND(ID_VIEW_APPLOOK_MDI_TABS, OnViewAppLookMDITabs)
	ON_UPDATE_COMMAND_UI(ID_VIEW_APPLOOK_MDI_TABS, OnUpdateViewAppLookMDITabs)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_CASCADE, OnUpdateWindowCascade)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_TILE_VERT, OnUpdateWindowCascade)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_TILE_HORZ, OnUpdateWindowCascade)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_ARRANGE, OnUpdateWindowCascade)
	ON_MESSAGE(UWM_HANDLEDLGKEY, OnHandleDlgKey)
	ON_MESSAGE(UWM_MODELESSDESTROY, OnModelessDestroy)
	ON_MESSAGE(UWM_PROPERTY_CHANGE, OnPropertyChange)
	ON_MESSAGE(UWM_PROPERTY_SELECT, OnPropertySelect)
	ON_MESSAGE(UWM_PROPERTY_HELP, OnPropertyHelp)
	ON_MESSAGE(UWM_PALETTE_CHANGE, OnPaletteChange)
	ON_MESSAGE(UWM_PALETTE_SELECTION, OnPaletteSelection)
	ON_MESSAGE(UWM_DEFERRED_SIZING, OnDeferredSizing)
	ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
	ON_COMMAND(ID_VIEW_RECORD_STATUS, OnViewRecordStatus)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RECORD_STATUS, OnUpdateViewRecordStatus)
	ON_COMMAND(ID_NEXT_PANE, OnNextPane)
	ON_COMMAND(ID_PREV_PANE, OnPrevPane)
END_MESSAGE_MAP()

// CMainFrame message handlers

void CMainFrame::OnWindowManager()
{
	ShowWindowsDialog();
}

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CMDIFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// base class does the real work

	if (!CMDIFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}

#if 0	// ck: disable toolbar customization for now
	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}
#endif

	// ck: disable UI customization for now, it's too confusing during development
#if _MFC_VER < 0xb00
	m_wndMenuBar.RestoreOriginalstate();
	m_wndToolBar.RestoreOriginalstate();
#else	// MS fixed typo
	m_wndMenuBar.RestoreOriginalState();
	m_wndToolBar.RestoreOriginalState();
#endif
	theApp.GetKeyboardManager()->ResetAll();
	theApp.GetContextMenuManager()->ResetState();

	return TRUE;
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
}

void CMainFrame::OutputText(const CString& txt)
{
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		switch (pMsg->wParam) {
		case VK_ESCAPE:
			// base class exits full screen but fails to restore MDI tabs
			FullScreen(false);
			break;
		default:
			CEdit	*pEdit = CFocusEdit::GetEdit();
			if (pEdit != NULL) {	// if edit control has focus
				switch (pMsg->wParam) {	// if arrow key
				case VK_UP:
				case VK_DOWN:
				case VK_LEFT:
				case VK_RIGHT:
					// pass key directly to edit control, bypassing accelerators 
					pEdit->SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
					return true;	// don't dispatch any further
				}
			}
		}
	}
	return CMDIFrameWndEx::PreTranslateMessage(pMsg);
}

LRESULT CMainFrame::OnAfterTaskbarActivate(WPARAM wParam, LPARAM lParam)
{
//	CMDIFrameWndEx::OnAfterTaskbarActivate(wParam, lParam);	// don't call base class method

	// The base class implementation of this handler triggers an excessive number
	// of unnecessary repaints when the app is restored from the iconic state by
	// clicking the taskbar. This is a known bug in CMDIFrameWndEx. See this post:
	//
	// OnAfterTaskbarActivate triggers multiple redraws on CustomDraw CListCtrl
	// https://connect.microsoft.com/VisualStudio/feedback/details/2474039/onaftertaskbaractivate-triggers-multiple-redraws-on-customdraw-clistctrl
	//
	// The following is copied from the base class implementation, but with all
	// lines that redraw commented out, as suggested in the post above.

//	AdjustDockingLayout();
//	RecalcLayout();

//	SetWindowPos(NULL, -1, -1, -1, -1, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
//	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE);

//	m_dockManager.RedrawAllMiniFrames();

	HWND hwndMDIChild = (HWND)lParam;
	if (hwndMDIChild != NULL && ::IsWindow(hwndMDIChild))
	{
		::SetFocus(hwndMDIChild);
	}
	return 0;
}

void CMainFrame::OnClose()
{
	CMDIFrameWndEx::OnClose();
}

LRESULT	CMainFrame::OnHandleDlgKey(WPARAM wParam, LPARAM lParam)
{
	return theApp.HandleDlgKeyMsg((MSG *)wParam);
}

LRESULT CMainFrame::OnModelessDestroy(WPARAM wParam, LPARAM lParam)
{
	CDialog	*pDlg = reinterpret_cast<CDialog *>(wParam);
	if (pDlg == m_pRecordStatusDlg) {
		m_pRecordStatusDlg = NULL;	// avoid leak
	}
	return 0;
}

LRESULT CMainFrame::OnPropertyChange(WPARAM wParam, LPARAM lParam)
{
	CPotterDrawDoc	*pDoc = GetActiveMDIDoc();
	if (pDoc != NULL) {
		int	iProp = INT64TO32(wParam);
		CWnd	*pWnd = reinterpret_cast<CWnd *>(lParam);
		if (pWnd == &m_wndPropertiesBar) {	// if notifier is properties bar
			pDoc->NotifyUndoableEdit(iProp, UCODE_PROPERTY);
			m_wndPropertiesBar.GetProperties(*pDoc);
			pDoc->m_arrPalette.SetSize(pDoc->m_nColors);	// in case color count changed
			if (pDoc->m_iPaletteCurSel >= pDoc->m_nColors)
				pDoc->m_iPaletteCurSel = -1;
			CPotterDrawDoc::CPropertyHint	hint(iProp);
			CView	*pSender = reinterpret_cast<CView *>(this);
			pDoc->UpdateAllViews(pSender, CPotterDrawDoc::HINT_PROPERTY, &hint);
			pDoc->SetModifiedFlag();
		} else if (pWnd == &m_wndModulationBar) {	// if notifier is modulation bar
			if (iProp == CModulationProps::PROP_iTarget) {	// if target changed
				CModulationProps	props;
				m_wndModulationBar.GetProperties(props);
				pDoc->m_iModTarget = props.m_iTarget;
				UpdateModulationBar(pDoc);
			} else {	// ordinary modulation property changed
				int	iTarget = pDoc->m_iModTarget;
				if (iTarget >= 0) {	// if current target is valid
					pDoc->NotifyUndoableEdit(MAKELONG(iProp, iTarget), UCODE_MODULATION);
					m_wndModulationBar.GetProperties(pDoc->m_Mod[iTarget]);	// after undo notification
					CPotterDrawDoc::CModulationHint	hint(iTarget, iProp);
					CView	*pSender = reinterpret_cast<CView *>(this);
					pDoc->UpdateAllViews(pSender, CPotterDrawDoc::HINT_MODULATION, &hint);
					pDoc->SetModifiedFlag();
				}
			}
		}
	}
	return 0;
}

LRESULT CMainFrame::OnPropertySelect(WPARAM wParam, LPARAM lParam)
{
	CPotterDrawDoc	*pDoc = GetActiveMDIDoc();
	if (pDoc != NULL) {
		int	iProp = INT64TO32(wParam);
		CWnd	*pWnd = reinterpret_cast<CWnd *>(lParam);
		if (pWnd == &m_wndPropertiesBar) {
			if (iProp != pDoc->m_iModTarget) {	// if target changed
				pDoc->m_iModTarget = iProp;
				UpdateModulationBar(pDoc);
			}
		}
	}
	return 0;
}

LRESULT CMainFrame::OnPropertyHelp(WPARAM wParam, LPARAM lParam)
{
	int	iProp = INT64TO32(wParam);
	CWnd	*pWnd = reinterpret_cast<CWnd*>(lParam);
	int	nID = 0;
	if (pWnd == &m_wndPropertiesBar) {	// if notifier is properties bar
		if (iProp >= 0 && iProp < CPotProperties::PROPERTIES)	// if valid property index
			nID = CPotProperties::m_Info[iProp].nNameID;	// get property name resource ID
		else
			nID = IDS_PROPERTIES_BAR;
	} else if (pWnd == &m_wndModulationBar) {	// if notifier is modulation bar
		if (iProp >= 0 && iProp < CModulationProps::PROPERTIES)	// if valid property index
			nID = CModulationProps::m_Info[iProp].nNameID;	// get property name resource ID
		else
			nID = IDS_MODULATION_BAR;
	}
	theApp.WinHelp(nID);
	return 0;
}

LRESULT CMainFrame::OnPaletteChange(WPARAM wParam, LPARAM lParam)
{
	CPotterDrawDoc	*pDoc = GetActiveMDIDoc();
	if (pDoc != NULL) {
		int	iColor = INT64TO32(wParam);
		int	cNew = UINT64TO32(lParam);
		pDoc->NotifyUndoableEdit(iColor, UCODE_PALETTE);
		if (iColor >= 0)	// if valid color index
			pDoc->m_arrPalette[iColor] = cNew;
		else {	// entire palette changed; size may have changed
			m_wndPaletteBar.GetPalette(pDoc->m_arrPalette);
			pDoc->m_nColors = pDoc->m_arrPalette.GetSize();
		}
		CView	*pSender = reinterpret_cast<CView *>(this); 
		pDoc->UpdateAllViews(pSender, CPotterDrawDoc::HINT_PALETTE);
		pDoc->SetModifiedFlag();
	}
	return 0;
}

LRESULT CMainFrame::OnPaletteSelection(WPARAM wParam, LPARAM lParam)
{
	CPotterDrawDoc	*pDoc = GetActiveMDIDoc();
	if (pDoc != NULL) {
		pDoc->m_iPaletteCurSel = INT64TO32(wParam);
	}
	return 0;
}

LRESULT CMainFrame::OnDeferredSizing(WPARAM wParam, LPARAM lParam)
{
	m_bDeferredSizing = false;
	CWaitCursor	wc;
	CAllViewIter	iter;
	CView	*pView;
	while ((pView = iter.GetNextView()) != NULL) {
		CRect	rView;
		pView->GetClientRect(rView);
		CSize	szView = rView.Size();
		pView->SendMessage(WM_SIZE, 0, MAKELONG(szView.cx, szView.cy));
	}
	return 0;
}

void CMainFrame::OnUpdateFilePrintSetup(CCmdUI *pCmdUI)
{
}

void CMainFrame::OnUpdateIndicatorZoom(CCmdUI *pCmdUI)
{
	CString	sZoom;
	if (m_pActiveView != NULL) {
		double	fZoom = m_pActiveView->GetZoom();
		sZoom.Format(_T("%g"), fZoom * 100);
	}
	pCmdUI->SetText(sZoom);
	pCmdUI->Enable(true);
}

void CMainFrame::OnUpdateIndicatorFrameRate(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pActiveView != NULL && (m_pActiveView->IsAnimating() || m_pActiveView->IsRecording()));
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	CMDIFrameWndEx::OnTimer(nIDEvent);
	if (nIDEvent == FRAME_RATE_TIMER) {
		CString	sFrameRate;
		if (m_pActiveView != NULL && (m_pActiveView->IsAnimating() || m_pActiveView->IsRecording())) {
			double	fFrameRate = m_pActiveView->MeasureFrameRate();
			sFrameRate.Format(_T("%.2f"), fFrameRate);
		}
		m_wndStatusBar.SetPaneText(INDICATOR_FRAME_RATE, sFrameRate);
	}
}

void CMainFrame::OnUpdateIndicatorResolution(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pActiveView != NULL);
	CString	sRes(GetResolutionString());
	pCmdUI->SetText(sRes);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIFrameWndEx::OnSize(nType, cx, cy);
	CString	sRes(GetResolutionString());
	m_wndStatusBar.SetPaneText(INDICATOR_RESOLUTION, sRes);
}

void CMainFrame::OnNextPane()
{
	GotoNextPane();
}

void CMainFrame::OnPrevPane()
{
	GotoNextPane(true);
}

void CMainFrame::OnViewAppLookMDITabs()
{
	m_bMDITabs ^= 1;
	EnableMDITabbedGroups(m_bMDITabs, m_mdiTabParams);
}

void CMainFrame::OnUpdateViewAppLookMDITabs(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bMDITabs);
}

void CMainFrame::OnViewRecordStatus()
{
	ShowRecordStatusDlg(m_pRecordStatusDlg == NULL);
}

void CMainFrame::OnUpdateViewRecordStatus(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pRecordStatusDlg != NULL);
}

void CMainFrame::OnToolsOptions()
{
	COptionsDlg	dlg;
	if (dlg.DoModal() == IDOK) {
		UpdateOptions();
	}
}

void CMainFrame::OnUpdateWindowCascade(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_bMDITabs);
}

void CMainFrame::OnWindowFullScreen()
{
	FullScreen(!IsFullScreen());
}
