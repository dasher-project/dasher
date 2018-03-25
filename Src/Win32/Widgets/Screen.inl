// Screen.inl
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002 David Ward
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

inline void CScreen::DrawRectangle(screenint x1, screenint y1, screenint x2, screenint y2, int Colour, int iOutlineColour, int iThickness) {
  HBRUSH brush = CScreen::GetBrush(Colour);
  RECT Rect;
  Rect.left = x1;
  Rect.top = y1;
  Rect.right = x2;
  Rect.bottom = y2;
  if(Colour != -1)
    FillRect(m_hDCBuffer, &Rect, brush);

  if(GetWidth() != (screenint)-1 && iThickness != 0) {

    point aPoints[5];

    aPoints[0].x=x1; aPoints[0].y=y1;
    aPoints[1].x=x2; aPoints[1].y=y1;
    aPoints[2].x=x2; aPoints[2].y=y2;
    aPoints[3].x=x1; aPoints[3].y=y2;
    aPoints[4].x=x1; aPoints[4].y=y1;

	Polyline(aPoints, 5, iThickness, iOutlineColour==-1 ? 3 : iOutlineColour);
  }
}

inline void CScreen::DrawCircle(screenint iCX, screenint iCY, screenint iR, int iFillColour, int iLineColour, int iThickness) {
  HGDIOBJ hpOld;
  hpOld = (HPEN) SelectObject(m_hDCBuffer, GetPen(iLineColour, iThickness));

  if(iFillColour!=-1) {
    HBRUSH hBrush = CScreen::GetBrush(iFillColour);
    HBRUSH hBrushOld;
    hBrushOld = (HBRUSH)SelectObject(m_hDCBuffer, hBrush);
  
    Ellipse(m_hDCBuffer, iCX - iR, iCY - iR, iCX + iR, iCY + iR);

    SelectObject(m_hDCBuffer, hBrushOld);
  }
  // TODO: Fix this (?) - looks to take no account of iThickness...and allegedly doesn't work on winCE either!
  if (iThickness>0)
    Arc(m_hDCBuffer, iCX - iR, iCY - iR, iCX + iR, iCY + iR,
                     iCX, iCY - iR, iCX, iCY - iR );

  SelectObject(m_hDCBuffer, hpOld);
}

inline void CScreen::Polyline(point *Points, int Number, int iWidth, int iColour) {
  HGDIOBJ hpOld;
  hpOld = (HPEN) SelectObject(m_hDCBuffer, GetPen(iColour, iWidth));
  POINT *WinPoints = new POINT[Number];
  point2POINT(Points, WinPoints, Number);
  ::Polyline(m_hDCBuffer, WinPoints, Number);
  delete[]WinPoints;
  SelectObject(m_hDCBuffer, hpOld);
}

inline void CScreen::Display() {
 // BitBlt(m_hdc, 0, 0, m_iWidth, m_iHeight, m_hDCBuffer, 0, 0, SRCCOPY);
  InvalidateRect(m_hWnd, NULL, false);
}

inline void CScreen::RealDisplay(HDC hDC, RECT r) {
  BitBlt(hDC, r.left, r.top, r.right - r.left, r.bottom - r.top, m_hDCBuffer, r.left, r.top, SRCCOPY);
}

inline const void CScreen::point2POINT(const point *In, POINT *Out, int Number) {
  // Yuck!
  for(int i = 0; i < Number; i++) {
    Out[i].x = In[i].x;
    Out[i].y = In[i].y;
  }
}

inline HPEN& CScreen::GetPen(int iColor, int iWidth) {
  int key = iColor+iWidth*256;

  auto hm1_RcIter = m_cPens.find( key );
  if( hm1_RcIter == m_cPens.end() ) {
    HPEN pen = ::CreatePen(PS_SOLID, iWidth, RGB(m_pColours->Reds[iColor], m_pColours->Greens[iColor], m_pColours->Blues[iColor]));
    m_cPens[key] = pen;
  }
  
  return m_cPens[key];
}

inline HBRUSH& CScreen::GetBrush(int iColor) {
  int key = iColor;
  // TODO: fix this hack. Why is iColor sometimes negative (-1)?
  if(key<0)
  {
    // This hack is here to prevent unchecked subcript access with negative
    // index to the colour vectors below,
    // which causes crashes.
    m_cBrushes[key] = (HBRUSH)GetStockObject(GRAY_BRUSH);
  }
  ////////////////////////////////////////////////////////

  auto hm1_RcIter = m_cBrushes.find( key );
  if( hm1_RcIter == m_cBrushes.end() ) {
    HBRUSH brush = CreateSolidBrush(RGB(m_pColours->Reds[iColor], m_pColours->Greens[iColor], m_pColours->Blues[iColor]));
    m_cBrushes[key] = brush;
  }
  
  return m_cBrushes[key];
}

inline HFONT& CScreen::GetFont(int iSize) {
  if (iSize > 50) // ???? Is there a limit to size, should it be a setting?
    iSize = 50;

  int key = iSize;

  std::wstring wstrOutput;
  WinUTF8::UTF8string_to_wstring(FontName, wstrOutput);

  auto hm1_RcIter = m_cFonts.find( key );
  if( hm1_RcIter == m_cFonts.end() ) {
    HFONT font = CreateFont(int (-iSize), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, LPCWSTR(wstrOutput.c_str())); // DEFAULT_CHARSET => font made just from Size and FontName
    m_cFonts[key] = font;
  }
  
  return m_cFonts[key];
}
