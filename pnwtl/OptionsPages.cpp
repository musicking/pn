#include "stdafx.h"
#include "OptionsPages.h"

//////////////////////////////////////////////////////////////////////////////
// CStyleDisplay
//////////////////////////////////////////////////////////////////////////////

CStyleDisplay::CStyleDisplay()
{
	m_Font = NULL;
	memset(&m_lf, 0, sizeof(LOGFONT));
}

CStyleDisplay::~CStyleDisplay()
{
	if(m_Font)
		delete m_Font;
}

void CStyleDisplay::SetBold(bool bold)
{
	m_lf.lfWeight = (bold ? FW_BOLD : FW_NORMAL);
	UpdateFont();
}

void CStyleDisplay::SetItalic(bool italic)
{
	m_lf.lfItalic = italic;
	UpdateFont();
}

void CStyleDisplay::SetUnderline(bool underline)
{
	m_lf.lfUnderline = underline;
	UpdateFont();
}

void CStyleDisplay::SetFontName(LPCTSTR fontname)
{
	_tcscpy(m_lf.lfFaceName, fontname);
	UpdateFont();
}

void CStyleDisplay::SetSize(int size, bool bInvalidate)
{
	HDC dc = GetDC();			
	m_lf.lfHeight = -MulDiv(size, GetDeviceCaps(dc, LOGPIXELSY), 72);
	ReleaseDC(dc);

	if(bInvalidate)
		UpdateFont();
}

void CStyleDisplay::SetFore(COLORREF fore)
{
	m_Fore = fore;
	Invalidate();
}

void CStyleDisplay::SetBack(COLORREF back)
{
	m_Back = back;
	Invalidate();
}

void CStyleDisplay::SetStyle(LPCTSTR fontname, int fontsize, COLORREF fore, COLORREF back, LPCTSTR name, bool bold, bool italic, bool underline)
{
	m_Name = name;
	m_Fore = fore;
	m_Back = back;

	SetSize(fontsize, false);
	
	m_lf.lfWeight = (bold ? FW_BOLD : FW_NORMAL);
	m_lf.lfUnderline = underline;
	m_lf.lfItalic = italic;
	_tcscpy(m_lf.lfFaceName, fontname);

	UpdateFont();
}

LRESULT CStyleDisplay::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PAINTSTRUCT ps;
	BeginPaint(&ps);

	CDC dc(ps.hdc);
	
	CRect rc;
	GetClientRect(rc);

	dc.FillRect(rc, (HBRUSH)::GetStockObject(WHITE_BRUSH));

	// Draw in the example text.
	if(m_Font)
	{
		HFONT hOldFont = dc.SelectFont(m_Font->m_hFont);
		dc.SetBkColor(m_Back);
		dc.SetTextColor(m_Fore);
		dc.DrawText(m_Name, m_Name.GetLength(), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		dc.SelectFont(hOldFont);
	}

	// Draw a light border around the control.
	HBRUSH light = ::GetSysColorBrush(COLOR_3DSHADOW);
	dc.FrameRect(rc, light);
	
	EndPaint(&ps);
	return 0;
}

LRESULT CStyleDisplay::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return 1;
}

void CStyleDisplay::UpdateFont()
{
	if(m_Font)
		delete m_Font;

	m_Font = new CFont;
	m_Font->CreateFontIndirect(&m_lf);

	Invalidate();
}

//////////////////////////////////////////////////////////////////////////////
// CTabPageKeywords
//////////////////////////////////////////////////////////////////////////////

CTabPageKeywords::CTabPageKeywords()
{
	m_pSet = NULL;
	m_bChanging = false;
}

void CTabPageKeywords::SetScheme(SchemeConfig* pScheme)
{
	m_pSet = NULL;
	m_pScheme = pScheme;

	// Set Keywords
	if( ::IsWindow(m_hWnd) )
	{
		DoSetScheme();
	}
}

void CTabPageKeywords::DoSetScheme()
{
	m_bChanging = true;

	SetItem();

	m_list.DeleteAllItems();

	CustomKeywordSet* pSet = m_pScheme->GetFirstKeywordSet();
	
	int iPos = 0;

	while(pSet)
	{
		if(pSet->pName)
		{
			int iItem = m_list.AddItem(iPos++, 0, pSet->pName);
			m_list.SetItemData(iItem, reinterpret_cast<DWORD>(pSet));
		}
		
		pSet = pSet->pNext;
	}

	m_list.SelectItem(0);

	m_bChanging = false;

	UpdateSel();
}

void CTabPageKeywords::SetItem()
{
	if(m_pSet)
	{
		// compare the keyword sets first, has the current one changed?
		int len = m_scintilla.GetTextLength();
		TCHAR* pCS = new TCHAR[len+1];
		m_scintilla.GetText(len+1, pCS);
		pCS[len] = _T('\0');
		
		if(_tcscmp(m_pSet->pWords, pCS) != 0)
		{
			CustomKeywordSet* pCustomSet = m_pScheme->m_cKeywords.FindKeywordSet(m_pSet->key);

			if(pCustomSet)
			{
				delete [] pCustomSet->pWords;
				pCustomSet->pWords = pCS;
				pCS = NULL;
			}
			else
			{
				CustomKeywordSet* pNewSet = new CustomKeywordSet(*m_pSet);
				pNewSet->pWords = pCS;
				pCS = NULL;
				m_pScheme->m_cKeywords.AddKeywordSet(pNewSet);
			}
		}
		else
		{
			CustomKeywordSet* pCustomSet = m_pScheme->m_cKeywords.FindKeywordSet(m_pSet->key);
			if(pCustomSet)
				m_pScheme->m_cKeywords.DeleteKeywordSet(pCustomSet);
		}

		if(pCS)
			delete [] pCS;
	}
}

void CTabPageKeywords::Finalise()
{
	// Ensure everything is saved that should be...
	SetItem();
}

void CTabPageKeywords::UpdateSel()
{
	if(!m_bChanging)
	{
		SetItem();

		int iSelected = m_list.GetSelectedIndex();
		if(iSelected != -1)
		{
			CustomKeywordSet* pRealSet = reinterpret_cast<CustomKeywordSet*>( m_list.GetItemData(iSelected) );

			if(pRealSet)
			{
				m_pSet = pRealSet;

				CustomKeywordSet* pS = pRealSet;
				CustomKeywordSet* pCustomSet = m_pScheme->m_cKeywords.FindKeywordSet(m_pSet->key);

				if(pCustomSet)
					pS = pCustomSet;

				if(pS->pWords)
				{
					m_scintilla.SetText(pS->pWords);
					EnableControls();
				}
			}
			else
				EnableControls(FALSE);
		}
		else
		{
			EnableControls(FALSE);
		}
	}
}

void CTabPageKeywords::EnableControls(BOOL bEnable)
{
	m_ResetBtn.EnableWindow(bEnable);
	m_SortBtn.EnableWindow(bEnable);
}

LRESULT CTabPageKeywords::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_ResetBtn.Attach(GetDlgItem(IDC_KEYWORDS_RESETBUTTON));
	m_SortBtn.Attach(GetDlgItem(IDC_KEYWORDS_SORTBUTTON));
	m_list.Attach(GetDlgItem(IDC_KEYWORDS_LIST));

	CRect rcScintilla;
	::GetWindowRect(GetDlgItem(IDC_PLACEHOLDER), rcScintilla);
	ScreenToClient(rcScintilla);
	m_scintilla.Create(m_hWnd, rcScintilla, "Keywords", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS);
	m_scintilla.SetWrapMode(SC_WRAP_WORD);
	m_scintilla.AssignCmdKey(SCK_HOME, SCI_HOMEDISPLAY);
	m_scintilla.AssignCmdKey(SCK_END, SCI_LINEENDDISPLAY);
	
	// Stop scintilla from capturing the escape and tab keys...
	m_scintilla.ClearCmdKey(SCK_ESCAPE);
	m_scintilla.ClearCmdKey(SCK_TAB);

	CRect rc;
	m_list.GetClientRect(&rc);
	int wCol = rc.right - rc.left - 20;
	m_list.InsertColumn(0, _T(""), LVCFMT_LEFT, wCol, 0);
	
	EnableControls(FALSE);

	if(m_pScheme)
		DoSetScheme();

	m_list.Invalidate(TRUE);

	return 0;
}

LRESULT CTabPageKeywords::OnResetClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pSet)
	{
		m_scintilla.SetText(m_pSet->pWords);
	}
	return 0;
}

#include <algorithm>

LRESULT CTabPageKeywords::OnSortClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	string str;

	int len = m_scintilla.GetTextLength();
	TCHAR* pCS = new TCHAR[len+1];
	m_scintilla.GetText(len+1, pCS);
	pCS[len] = _T('\0');
	str = pCS;
	delete [] pCS;

	vector<string> tokens;

	StringTokenise(str, tokens);
	
	std::sort(tokens.begin(), tokens.end());

	string strout;
	strout.reserve(len+1);

	for(vector<string>::iterator i = tokens.begin(); i != tokens.end(); ++i)
	{
		if(i != tokens.begin())
			strout += _T(" ");
		strout += (*i);
	}

	m_scintilla.SetText(strout.c_str());

	return 0;
}

LRESULT CTabPageKeywords::OnListSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	UpdateSel();

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// CTabPageStyles
//////////////////////////////////////////////////////////////////////////////

CTabPageStyles::CTabPageStyles()
{
	m_pStyle = NULL;
	m_bChanging = false;
}

void CTabPageStyles::SetScheme(SchemeConfig* pScheme)
{
	m_bChanging = true;
	m_pStyle = NULL;
	m_pScheme = pScheme;

	m_tree.DeleteAllItems();

	CustomStyleCollection* pColl = static_cast<CustomStyleCollection*>(pScheme);
	HTREEITEM insertunder = TVI_ROOT;
	
	while(pColl)
	{
		for(SL_IT i = pColl->m_Styles.begin(); i != pColl->m_Styles.end(); ++i)
		{
			HTREEITEM hi = m_tree.InsertItem((*i)->name.c_str(), insertunder, TVI_LAST);
			m_tree.SetItemData(hi, reinterpret_cast<DWORD_PTR>(*i));
		}
		if(insertunder != TVI_ROOT)
			m_tree.Expand(insertunder, TVE_EXPAND);

		pColl = pColl->GetNext();
		if(pColl)
		{
			insertunder = m_tree.InsertItem(pColl->GetName(), NULL, NULL);
			m_tree.SetItemData(insertunder, reinterpret_cast<DWORD_PTR>(pColl));
		}
	}

	m_tree.EnsureVisible(m_tree.GetRootItem());

	m_bChanging = false;
}

void CTabPageStyles::Finalise()
{
	SetItem();
}

LRESULT CTabPageStyles::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CRect rc;
	
	m_tree.Attach(GetDlgItem(IDC_STYLES_TREE));

	CWindow placeholder(GetDlgItem(IDC_STYLE_EXAMPLE));
	placeholder.GetWindowRect(rc);
	ScreenToClient(rc);
	m_sd.Create(m_hWnd, rc, _T("Style Display"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS);

	m_FontCombo.SubclassWindow(GetDlgItem(IDC_STYLE_FONTCOMBO));
	m_SizeCombo.Attach(GetDlgItem(IDC_STYLE_SIZECOMBO));

	m_fore.SubclassWindow(GetDlgItem(IDC_STYLE_FOREBUTTON));
	m_back.SubclassWindow(GetDlgItem(IDC_STYLE_BACKBUTTON));

	m_fore.SetDefaultColor(RGB(0,0,0));
	m_back.SetDefaultColor(RGB(255,255,255));
	
	m_SizeCombo.Add(6);
	m_SizeCombo.Add(8);
	m_SizeCombo.Add(10);
	m_SizeCombo.Add(12);
	m_SizeCombo.Add(14);
	m_SizeCombo.Add(16);
	m_SizeCombo.Add(18);

	m_bold.Attach(GetDlgItem(IDC_STYLE_BOLDCHECK));
	m_italic.Attach(GetDlgItem(IDC_STYLE_ITALICCHECK));
	m_underline.Attach(GetDlgItem(IDC_STYLE_UNDERLINECHECK));
	m_eolfilled.Attach(GetDlgItem(IDC_STYLE_EOLFILLEDCHECK));

	return 0;
}

LRESULT CTabPageStyles::OnForeChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NMCOLORBUTTON* pN = reinterpret_cast<NMCOLORBUTTON*>(pnmh);
	COLORREF col = (pN->clr == CLR_DEFAULT ? m_fore.GetDefaultColor() : pN->clr);
	m_sd.SetFore(col);
	m_Style.ForeColor = col;
	return 0;
}

LRESULT CTabPageStyles::OnBackChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NMCOLORBUTTON* pN = reinterpret_cast<NMCOLORBUTTON*>(pnmh);
	COLORREF col = (pN->clr == CLR_DEFAULT ? m_fore.GetDefaultColor() : pN->clr);
	m_sd.SetBack(col);
	m_Style.BackColor = col;
	return 0;
}

LRESULT CTabPageStyles::OnFontChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{			
	if(m_pStyle)
	{
		int i = m_FontCombo.GetCurSel();
		CString str;
		m_FontCombo.GetLBText(i, str);
		m_sd.SetFontName(str);
		m_Style.FontName = (LPCTSTR)str;
	}
	return 0;
}

LRESULT CTabPageStyles::OnSizeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pStyle)
	{
		int i = m_SizeCombo.GetSelection();
		m_sd.SetSize(i);
		m_Style.FontSize = i;
	}
	return 0;
}

LRESULT CTabPageStyles::OnTreeSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	UpdateSel();

	return 0;
}

void CTabPageStyles::UpdateSel()
{
	// If we're not in the middle of changing scheme...
	if(!m_bChanging)
	{
		SetItem();

		HTREEITEM item = m_tree.GetSelectedItem();

		if(item)
		{
			if(m_tree.GetChildItem(item) == NULL)
			{
				StyleDetails* pS = reinterpret_cast<StyleDetails*>(m_tree.GetItemData(item));
				if(pS)
				{
					StyleDetails* existing = m_pScheme->m_customs.GetStyle(pS->Key);
					if(existing)
						m_Style = *existing;
					else
						m_Style = *pS;
					
					m_pStyle = pS;
					m_sd.SetStyle(m_Style.FontName.c_str(), m_Style.FontSize, m_Style.ForeColor, m_Style.BackColor, m_Style.name.c_str(), m_Style.Bold, m_Style.Italic, m_Style.Underline);
					m_bold.SetCheck(m_Style.Bold ? BST_CHECKED : BST_UNCHECKED);
					m_italic.SetCheck(m_Style.Italic ? BST_CHECKED : BST_UNCHECKED);
					m_underline.SetCheck(m_Style.Underline ? BST_CHECKED : BST_UNCHECKED);
					m_eolfilled.SetCheck(m_Style.EOLFilled ? BST_CHECKED : BST_UNCHECKED);
					m_fore.SetColor(m_Style.ForeColor);
					m_back.SetColor(m_Style.BackColor);

					m_FontCombo.SelectString(-1, m_Style.FontName.c_str());
					m_SizeCombo.Select(m_Style.FontSize);
				}
			}
			else
			{
				///@todo Disable everything
			}
		}
		else
			m_pStyle = NULL;
	}
}

LRESULT CTabPageStyles::OnBoldClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pStyle)
	{
		bool bC = m_bold.GetCheck() == BST_CHECKED;
		m_Style.Bold = bC;
		m_sd.SetBold(bC);
	}

	return 0;
}

LRESULT CTabPageStyles::OnItalicClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pStyle)
	{
		bool bC = m_italic.GetCheck() == BST_CHECKED;
		m_Style.Italic = bC;
		m_sd.SetItalic(bC);
	}
	return 0;
}

LRESULT CTabPageStyles::OnUnderlineClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pStyle)
	{
		bool bC = m_underline.GetCheck() == BST_CHECKED;
		m_Style.Underline = bC;
		m_sd.SetUnderline(bC);
	}
	return 0;
}

LRESULT CTabPageStyles::OnEOLFilledClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pStyle)
	{
		bool bC = m_eolfilled.GetCheck() == BST_CHECKED;
		m_Style.EOLFilled = bC;
		//m_sd.SetEOLFilled(bC);
	}
	return 0;
}

void CTabPageStyles::SetItem()
{
	if(m_pStyle)
	{
		int mask = 0;

		if(m_Style != *m_pStyle)
		{
			/* The style the user has configured and the original definition version
			   do not match. We need to store the new style in the custom style
			   store. */
			StyleDetails* existing = m_pScheme->m_customs.GetStyle(m_Style.Key);
			if(existing)
			{
				*existing = m_Style;
			}
			else
			{
				existing = new StyleDetails;
				*existing = m_Style;
				m_pScheme->m_customs.AddStyle(existing);
			}
		}
		else
		{
			/* If we have set the style to be like the original, then
			   we can safely remove any custom styles. */
			m_pScheme->m_customs.RemoveStyle(m_Style.Key);
		}
	}
}

LRESULT CTabPageStyles::OnResetClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pScheme)
	{
		if(m_pStyle)
		{
			m_pScheme->m_customs.RemoveStyle(m_pStyle->Key);
			m_pStyle = NULL;
		}
	
		UpdateSel();
	}

	return 0;
}

LRESULT CTabPageStyles::OnResetAllClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_pScheme)
	{
		m_pStyle = NULL;
		m_pScheme->m_customs.RemoveAll();

		UpdateSel();
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// COptionsPageStyle
//////////////////////////////////////////////////////////////////////////////

LRESULT COptionsPageStyle::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_FontCombo.SubclassWindow(GetDlgItem(IDC_FONT_COMBO));
	m_SizeCombo.Attach(GetDlgItem(IDC_FONTSIZE_COMBO));

	m_fore.SubclassWindow(GetDlgItem(IDC_STYLE_FOREBUTTON));
	m_back.SubclassWindow(GetDlgItem(IDC_STYLE_BACKBUTTON));

	m_bold.Attach(GetDlgItem(IDC_STYLE_BOLDCHECK));
	m_italic.Attach(GetDlgItem(IDC_STYLE_ITALICCHECK));
	m_underline.Attach(GetDlgItem(IDC_STYLE_UNDERLINECHECK));
	
	m_SizeCombo.Add(6);
	m_SizeCombo.Add(8);
	m_SizeCombo.Add(10);
	m_SizeCombo.Add(12);
	m_SizeCombo.Add(14);
	m_SizeCombo.Add(16);
	m_SizeCombo.Add(18);

	return 0;
}

void COptionsPageStyle::OnInitialise()
{
	if(m_pSchemes)
	{
		StyleDetails* pStyle = m_pSchemes->GetDefaultStyle();
		
		m_FontCombo.SelectString(-1, pStyle->FontName.c_str());
		m_SizeCombo.Select(pStyle->FontSize);
		
		m_fore.SetColor(pStyle->ForeColor);
		m_fore.SetDefaultColor(RGB(0,0,0));
		
		m_back.SetColor(pStyle->BackColor);
		m_back.SetDefaultColor(RGB(255,255,255));

		m_bold.SetCheck(pStyle->Bold ? BST_CHECKED : BST_UNCHECKED);
		m_italic.SetCheck(pStyle->Italic ? BST_CHECKED : BST_UNCHECKED);
		m_underline.SetCheck(pStyle->Underline ? BST_CHECKED : BST_UNCHECKED);
	}
}

void COptionsPageStyle::OnOK()
{
	if(m_bCreated)
	{
		bool bIsCustom;
		StyleDetails* pCurrent = GetDefault(bIsCustom);
		StyleDetails* pS = new StyleDetails(*pCurrent);
		
		int i = m_FontCombo.GetCurSel();
		CString str;
		m_FontCombo.GetLBText(i, str);

		pS->FontName = str;
		pS->FontSize = m_SizeCombo.GetSelection();
		pS->ForeColor = m_fore.SafeGetColor();
		pS->BackColor = m_back.SafeGetColor();
		pS->Bold = (m_bold.GetCheck() == BST_CHECKED);
		pS->Italic = (m_italic.GetCheck() == BST_CHECKED);
		pS->Underline = (m_underline.GetCheck() == BST_CHECKED);

		if(*pS != *pCurrent)
		{
			// the new style is not the same as the current style...
			
			if(bIsCustom)
			{
				// the current style is already a custom one.
				StyleDetails* pOrig = m_pSchemes->GetStyleClasses().GetStyle(_T("default"));
				if(*pOrig == *pS)
				{
					// The user has reverted to the original style.
					m_pSchemes->GetCustomClasses().DeleteStyle(_T("default"));
				}
				else
				{
					// pCurrent is already in the "Custom" classes collection. Update it.
					*pCurrent = *pS;
				}

				/* If there was already a custom version of this style then one
				way or another, there is no need for our temporary one any more. */
				delete pS;
			}
			else
			{
				// There isn't already a custom style for this class, so we add one.
				m_pSchemes->GetCustomClasses().AddStyle(_T("default"), pS);
			}
		}
		else
		{
			delete pS;
		}
	}
}

void COptionsPageStyle::OnCancel()
{
}

LPCTSTR COptionsPageStyle::GetTreePosition()
{
	return _T("Style");
}

StyleDetails* COptionsPageStyle::GetDefault(bool& bIsCustom)
{
	bIsCustom = false;
	
	StyleDetails* pCustom = m_pSchemes->GetCustomClasses().GetStyle(_T("default"));
	if(pCustom)
	{
		bIsCustom = true;
		return pCustom;
	}
	
	return m_pSchemes->GetDefaultStyle();
}

//////////////////////////////////////////////////////////////////////////////
// COptionsPageSchemes
//////////////////////////////////////////////////////////////////////////////

COptionsPageSchemes::COptionsPageSchemes(SchemeConfigParser* pSchemes) : COptionsPageImpl<COptionsPageSchemes>()
{
	m_pSchemes = pSchemes;
}

LRESULT COptionsPageSchemes::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CWindow label;
	CSize s;
	CRect rc;

	label.Attach(GetDlgItem(IDC_SCHEMELABEL));
	
	CDC dc(label.GetDC());
	dc.GetTextExtent(_T("Scheme:"), 7, &s);
	
	label.GetWindowRect(rc);
	ScreenToClient(rc);
	rc.right = rc.left + s.cx;
	label.SetWindowPos(HWND_TOP, &rc, 0);

	CRect rcCombo;

	m_combo.Attach(GetDlgItem(IDC_SCHEMECOMBO));

	m_combo.GetWindowRect(rcCombo);
	ScreenToClient(rcCombo);
	rcCombo.left = rc.right + 5;
	m_combo.SetWindowPos(HWND_TOP, &rcCombo, 0);

	
	CRect rcPH;
	::GetWindowRect(GetDlgItem(IDC_PS_PLACEHOLDER), rcPH);
	ScreenToClient(rcPH);
	m_stylestab.SetTitle(_T("Styles"));
	m_keywordstab.SetTitle(_T("Keywords"));
	m_props.AddPage(m_stylestab);
	m_props.AddPage(m_keywordstab);
	
	m_props.Create(m_hWnd, 0, rcPH);

	return 0;
}

void COptionsPageSchemes::OnInitialise()
{
	for(SCF_IT i = m_pSchemes->GetSchemes().begin(); i != m_pSchemes->GetSchemes().end(); ++i)
	{
		int index = m_combo.AddString((*i)->m_Title);
		m_combo.SetItemDataPtr(index, (*i));
	}
	
	if(m_combo.GetCount() > 0)
	{
		m_combo.SetCurSel(0);
		Update();
	}
}

void COptionsPageSchemes::OnOK()
{
	m_stylestab.Finalise();
	m_keywordstab.Finalise();
	m_pSchemes->SaveConfig();
}

LPCTSTR COptionsPageSchemes::GetTreePosition()
{
	return _T("Style\\Schemes");
}

LRESULT COptionsPageSchemes::OnComboChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	Update();
	return 0;
}

void COptionsPageSchemes::Update()
{
	int i = m_combo.GetCurSel();
	SchemeConfig* pScheme = static_cast<SchemeConfig*>(m_combo.GetItemDataPtr(i));
	m_stylestab.SetScheme(pScheme);
	m_keywordstab.SetScheme(pScheme);
}

//////////////////////////////////////////////////////////////////////////////
// SchemeTools
//////////////////////////////////////////////////////////////////////////////

SchemeTools::SchemeTools(LPCTSTR schemename)
{
	m_Scheme = schemename;
}

SchemeTools::~SchemeTools()
{
	for(TOOLDEFS_LIST::iterator i = m_Tools.begin(); i != m_Tools.end(); ++i)
	{
		delete *i;
	}

	m_Tools.clear();
}

TOOLDEFS_LIST& SchemeTools::GetTools()
{
	return m_Tools;
}

void SchemeTools::Add(SToolDefinition* pDef)
{
	m_Tools.push_back(pDef);
}

void SchemeTools::Delete(SToolDefinition* pDef)
{
	m_Tools.remove(pDef);
	delete pDef;
}

void SchemeTools::WriteDefinition(ofstream& stream)
{
	if(m_Tools.size() != 0)
	{
		stream << "\t<scheme name=\"" << m_Scheme << "\">\n";
		
		for(TOOLDEFS_LIST::const_iterator i = m_Tools.begin(); i != m_Tools.end(); ++i)
		{
			stream << "\t\t<tool name=\"" << (*i)->Name << "\" ";
			stream << "command=\"" << (*i)->Command << "\" ";
			stream << "folder=\"" << (*i)->Folder << "\" ";
			stream << "params=\"" << (*i)->Params << "\" ";
			stream << "shortcut=\"" << (*i)->Shortcut << "\" ";
			stream << "/>\n";
		}

		stream << "\t</scheme>\n";
	}
}

//////////////////////////////////////////////////////////////////////////////
// SchemeToolsManager
//////////////////////////////////////////////////////////////////////////////

SchemeToolsManager::SchemeToolsManager()
{
	m_pCur = NULL;

	ReLoad();
}

SchemeToolsManager::~SchemeToolsManager()
{
	Clear();
}

void SchemeToolsManager::Clear()
{
	for(SCHEMETOOLS_MAP::iterator i = m_toolSets.begin(); i != m_toolSets.end(); ++i)
	{
		delete (*i).second;
	}

	m_toolSets.clear();
}

SchemeTools* SchemeToolsManager::GetToolsFor(LPCTSTR scheme)
{
	tstring stofind(scheme);
	SCHEMETOOLS_MAP::iterator i = m_toolSets.find(stofind);
	
	SchemeTools* pRet = NULL;

	if(i != m_toolSets.end())
	{
		pRet = (*i).second;
	}
	else
	{
		pRet = new SchemeTools(stofind.c_str());
		m_toolSets.insert(SCHEMETOOLS_MAP::value_type(stofind, pRet));
	}

	return pRet;
}

void SchemeToolsManager::ReLoad()
{
	XMLParser parser;
	parser.SetParseState(this);

	tstring uspath;
	COptionsManager::GetInstance()->GetPNPath(uspath, PNPATH_USERSETTINGS);
	uspath += _T("UserTools.xml");

	if(FileExists(uspath.c_str()))
	{
		try
		{
			parser.LoadFile(uspath.c_str());
		}
		catch ( XMLParserException& ex )
		{
			::OutputDebugString(_T("XML Parser Exception loading Scheme Tools:"));
			::OutputDebugString(ex.GetMessage());
		}

	}
}

void SchemeToolsManager::Save()
{
	tstring uspath;
	COptionsManager::GetInstance()->GetPNPath(uspath, PNPATH_USERSETTINGS);
	uspath += _T("UserTools.xml");

	ofstream str;
	str.open(uspath.c_str(), ios_base::out);
	if(str.is_open())
	{
		str << "<?xml version=\"1.0\"?>\n<schemetools>";
		for(SCHEMETOOLS_MAP::const_iterator i = m_toolSets.begin(); i != m_toolSets.end(); ++i)
		{
			(*i).second->WriteDefinition(str);
		}
		str << "</schemetools>";

		str.close();
	}
}

void SchemeToolsManager::processScheme(XMLAttributes& atts)
{
	LPCTSTR schemename = atts.getValue(_T("name"));
	if(schemename)
	{
		m_pCur = new SchemeTools(schemename);

		tstring stoadd(schemename);
		m_toolSets.insert(SCHEMETOOLS_MAP::value_type(stoadd, m_pCur));
	}
}

void SchemeToolsManager::processTool(XMLAttributes& atts)
{
	LPCTSTR toolname = atts.getValue(_T("name"));
	if(m_pCur && toolname)
	{
		SToolDefinition* pDef = new SToolDefinition;
		pDef->Name = toolname;
		
		int c = atts.getCount();

		for(int i = 0; i < c; ++i)
		{
			LPCTSTR attr = atts.getName(i);
			LPCTSTR val = atts.getValue(i);
			
			if(_tcscmp(attr, _T("command")) == 0)
				pDef->Command = val;
			else if(_tcscmp(attr, _T("params")) == 0)
				pDef->Params = val;
			else if(_tcscmp(attr, _T("folder")) == 0)
				pDef->Folder = val;
			else if(_tcscmp(attr, _T("shortcut")) == 0)
				pDef->Shortcut = val;
		}

		m_pCur->Add(pDef);
	}
}

void SchemeToolsManager::startElement(LPCTSTR name, XMLAttributes& atts)
{
	if(_tcscmp(name, _T("scheme")) == 0)
	{
		processScheme(atts);
	}
	else if(_tcscmp(name, _T("tool")) == 0)
	{
		processTool(atts);
	}
}

void SchemeToolsManager::endElement(LPCTSTR name)
{
	if(_tcscmp(name, _T("scheme")) == 0)
		m_pCur = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// COptionsPageTools
//////////////////////////////////////////////////////////////////////////////

COptionsPageTools::COptionsPageTools(SchemeConfigParser* pSchemes)
{
	m_pSchemes = pSchemes;
	m_pScheme = NULL;
	m_pCurrent = NULL;
}

COptionsPageTools::~COptionsPageTools()
{
	
}

LRESULT COptionsPageTools::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CWindow label;
	CSize s;
	CRect rc;

	label.Attach(GetDlgItem(IDC_SCHEMELABEL));
	
	CDC dc(label.GetDC());
	dc.GetTextExtent(_T("Scheme:"), 7, &s);
	
	label.GetWindowRect(rc);
	ScreenToClient(rc);
	rc.right = rc.left + s.cx;
	label.SetWindowPos(HWND_TOP, &rc, 0);

	CRect rcCombo;

	m_combo.Attach(GetDlgItem(IDC_SCHEMECOMBO));

	m_combo.GetWindowRect(rcCombo);
	ScreenToClient(rcCombo);
	rcCombo.left = rc.right + 5;
	m_combo.SetWindowPos(HWND_TOP, &rcCombo, 0);

	m_list.Attach(GetDlgItem(IDC_LIST));
	m_list.SetExtendedListViewStyle( m_list.GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT );
	m_list.GetClientRect(rc);
	m_list.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 130, 0);
	m_list.InsertColumn(1, _T("Command"), LVCFMT_LEFT, rc.Width() - 130 - 20, 0);

	m_btnMoveUp.SetDirection(CArrowButton::abdUp);
	m_btnMoveUp.SubclassWindow(GetDlgItem(IDC_TOOLS_MOVEUPBUTTON));
	m_btnMoveDown.SubclassWindow(GetDlgItem(IDC_TOOLS_MOVEDOWNBUTTON));

	EnableButtons();

	return 0;
}

void COptionsPageTools::OnInitialise()
{
	for(SCF_IT i = m_pSchemes->GetSchemes().begin(); i != m_pSchemes->GetSchemes().end(); ++i)
	{
		int index = m_combo.AddString((*i)->m_Title);
		m_combo.SetItemDataPtr(index, (*i));
	}
	
	if(m_combo.GetCount() > 0)
	{
		m_combo.SetCurSel(0);
		Update();
	}
}

void COptionsPageTools::OnOK()
{
	m_toolstore.Save();
}

void COptionsPageTools::Update()
{
	m_bChanging = true;

	m_pCurrent = NULL;
	m_list.DeleteAllItems();

	int iSel = m_combo.GetCurSel();
	if (iSel != -1)
	{
		m_pScheme = reinterpret_cast<SchemeConfig*>(m_combo.GetItemData(iSel));
	}
	else
		m_pScheme = NULL;

	SchemeTools* pTools = GetTools();
	if(pTools)
	{
		TOOLDEFS_LIST& l = pTools->GetTools();
		for(TOOLDEFS_LIST::const_iterator i = l.begin(); i != l.end(); ++i)
		{
			AddDefinition(*i);
		}
	}

	m_bChanging = false;
	
	EnableButtons();
}

SchemeTools* COptionsPageTools::GetTools()
{
	if(!m_pCurrent)
	{
		m_pCurrent = m_toolstore.GetToolsFor(m_pScheme->m_Name);
	}
		
	return m_pCurrent;
}

void COptionsPageTools::EnableButtons()
{
	if(m_bChanging)
		return;

	bool bEnable = (m_pScheme != NULL);
	int iSelIndex = m_list.GetSelectedIndex();

	::EnableWindow(GetDlgItem(IDC_TOOLS_ADDBUTTON), bEnable);
	
	// A scheme, and a selected item...
	bEnable = bEnable && (iSelIndex != -1);

	::EnableWindow(GetDlgItem(IDC_TOOLS_REMOVEBUTTON), bEnable);
	::EnableWindow(GetDlgItem(IDC_TOOLS_EDITBUTTON), bEnable);

	m_btnMoveUp.EnableWindow(bEnable && (iSelIndex != 0));
	m_btnMoveDown.EnableWindow(bEnable && (iSelIndex != (m_list.GetItemCount() - 1)));
}

void COptionsPageTools::AddDefinition(SToolDefinition* pDef)
{
	LVITEM lvi;

	lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = m_list.GetItemCount();
	lvi.iSubItem = 0;
	lvi.pszText = const_cast<LPTSTR>( pDef->Name.c_str() );
	lvi.iImage = 0;
	lvi.lParam = reinterpret_cast<LPARAM>(pDef);

	int iItem = m_list.InsertItem(&lvi);

	lvi.iItem = iItem;
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = 1;
	lvi.pszText = const_cast<LPTSTR>( pDef->Command.c_str() );
	m_list.SetItem(&lvi);
}

LRESULT COptionsPageTools::OnAddClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CToolEditorDialog dlg;
	
	if (dlg.DoModal() == IDOK)
	{
		//@todo check if the name is valid...

		SToolDefinition* pDef = new SToolDefinition;
		GetTools()->Add(pDef);
		dlg.GetValues(pDef);

		AddDefinition(pDef);
	}

	return 0;
}

LRESULT COptionsPageTools::OnEditClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iSelIndex = m_list.GetSelectedIndex();

	if(iSelIndex != -1)
	{
		SToolDefinition* pDef = reinterpret_cast<SToolDefinition*>(m_list.GetItemData(iSelIndex));
		if(pDef != NULL)
		{
			CToolEditorDialog dlg;
			dlg.SetValues(pDef);
			dlg.SetTitle(_T("Edit Tool"));

			if(dlg.DoModal())
			{
				dlg.GetValues(pDef);

				LVITEM lvi;
				lvi.mask = LVIF_TEXT;
				lvi.iItem = iSelIndex;
				lvi.iSubItem = 0;
				lvi.pszText = const_cast<LPTSTR>( pDef->Name.c_str() );
				m_list.SetItem(&lvi);

				lvi.iSubItem = 1;
				lvi.pszText = const_cast<LPTSTR>( pDef->Command.c_str() );
				m_list.SetItem(&lvi);
			}
		}
	}

	return 0;
}

LRESULT COptionsPageTools::OnRemoveClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iSelIndex = m_list.GetSelectedIndex();

	if(iSelIndex != -1)
	{
		SToolDefinition* pDef = reinterpret_cast<SToolDefinition*>(m_list.GetItemData(iSelIndex));
		if(pDef != NULL)
			GetTools()->Delete(pDef);
		m_list.DeleteItem(iSelIndex);
	}

	return 0;
}

LRESULT COptionsPageTools::OnComboChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	Update();

	return 0;
}

LPCTSTR COptionsPageTools::GetTreePosition()
{
	return _T("Tools");
}

LRESULT COptionsPageTools::OnListKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	EnableButtons();

	return 0;
}

LRESULT COptionsPageTools::OnListClicked(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	EnableButtons();

	return 0;
}

LRESULT COptionsPageTools::OnListDblClicked(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	BOOL bHandled;
	OnEditClicked(0, 0, 0, bHandled);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
// CToolEditorDialog
//////////////////////////////////////////////////////////////////////////////

CToolEditorDialog::CInfoLabel::CInfoLabel()
{
	m_pTitleFont = /*m_pBodyFont =*/ NULL;

	memset(strbuf, 0, sizeof(strbuf));
	LoadString(NULL, IDS_TOOLFORMATSTRINGS, strbuf, 200);
}

CToolEditorDialog::CInfoLabel::~CInfoLabel()
{
	if(m_pTitleFont)
	{
		delete m_pTitleFont;
		//delete m_pBodyFont;
	}
}

void CToolEditorDialog::CInfoLabel::MakeFonts(HDC hDC)
{
	if(!m_pTitleFont)
	{
		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));

		HFONT hDefFont = static_cast<HFONT>( GetStockObject(DEFAULT_GUI_FONT) );
		GetObject(hDefFont, sizeof(LOGFONT), &lf);
		
		lf.lfWeight = FW_BOLD;

		m_pTitleFont = new CFont;
		m_pTitleFont->CreateFontIndirect(&lf);
	}
	
	//lf.lfWeight = FW_NORMAL;
	//m_pBodyFont->CreateFontIndirect(&lf);
}

LRESULT CToolEditorDialog::CInfoLabel::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PAINTSTRUCT ps;
	::BeginPaint(m_hWnd, &ps);

	CDCHandle dc(ps.hdc);

	MakeFonts(dc);

	CRect rc;

	GetClientRect(rc);
	
	CRect framerc(rc);

	CBrush brush;
	brush.CreateSysColorBrush(COLOR_INFOBK);

	dc.FillRect(rc, brush);

	rc.DeflateRect(3, 3, 2, 2);

	// Draw Text...
	if(m_pTitleFont)
	{
		HFONT hOldFont = dc.SelectFont(m_pTitleFont->m_hFont);
		
		dc.SetBkColor(GetSysColor(COLOR_INFOBK));
		dc.SetTextColor(GetSysColor(COLOR_INFOTEXT));

		int height = dc.DrawText(_T("Special Symbols:"), 16, rc, DT_TOP | DT_LEFT);
		rc.top += height + 2;
		rc.left += 25;

		dc.SelectStockFont(DEFAULT_GUI_FONT);

		/* We draw up to two columns of text to display the % special chars. 
		This should be modified to draw as many as necessary. 
		Use a while pPipe instead of if...*/
		///@todo Make this x-column tastic.

		TCHAR* pPipe = _tcschr(strbuf, _T('|'));
		CRect rcCol(rc);
		if(pPipe)
		{
			*pPipe = '\0';
			dc.DrawText(strbuf, _tcslen(strbuf), rcCol, DT_TOP | DT_LEFT | DT_WORDBREAK | DT_CALCRECT);	
		}

		dc.DrawText(strbuf, _tcslen(strbuf), rc, DT_TOP | DT_LEFT | DT_WORDBREAK);

		if(pPipe)
		{
			*pPipe++ = _T('|');
			rc.left += rcCol.Width() + 20;
			dc.DrawText(pPipe, _tcslen(pPipe), rc, DT_TOP | DT_LEFT | DT_WORDBREAK);
		}

		dc.SelectFont(hOldFont);
	}

	HBRUSH light = ::GetSysColorBrush(COLOR_3DSHADOW);
	dc.FrameRect(framerc, light);

	::EndPaint(m_hWnd, &ps);
	return 0;
}

CToolEditorDialog::CToolEditorDialog()
{
	m_csName = _T("");
	m_csCommand = _T("");
	m_csFolder = _T("");
	m_csParams = _T("");
	m_csShortcut = _T("");

	m_csDisplayTitle = _T("New Tool");
}

LRESULT CToolEditorDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetWindowText(m_csDisplayTitle);

	m_infolabel.SubclassWindow(GetDlgItem(IDC_TE_INFOLABEL));

	DoDataExchange();

	return 0;
}

LRESULT CToolEditorDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DoDataExchange(TRUE);
	EndDialog(wID);

	return 0;
}

LRESULT CToolEditorDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);

	return 0;
}

void CToolEditorDialog::GetValues(SToolDefinition* pDefinition)
{
	pDefinition->Name		= m_csName;
	pDefinition->Command	= m_csCommand;
	pDefinition->Folder		= m_csFolder;
	pDefinition->Params		= m_csParams;
	pDefinition->Shortcut	= m_csShortcut;
}

void CToolEditorDialog::SetValues(SToolDefinition* pDefinition)
{
	m_csName		= pDefinition->Name.c_str();
	m_csCommand		= pDefinition->Command.c_str();
	m_csFolder		= pDefinition->Folder.c_str();
	m_csParams		= pDefinition->Params.c_str();
	m_csShortcut	= pDefinition->Shortcut.c_str();
}

void CToolEditorDialog::SetTitle(LPCTSTR title)
{
	m_csDisplayTitle = title;
}