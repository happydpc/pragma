#include "stdafx_client.h"
#include "pragma/clientstate/clientstate.h"
#include "pragma/gui/mainmenu/wimainmenu_base.h"
#include "pragma/gui/mainmenu/wimainmenu.h"
#include <wgui/types/wibutton.h>
#include "pragma/gui/wioptionslist.h"
#include <wgui/types/witext.h>
#include <wgui/types/wirect.h>
#include "pragma/localization.h"

LINK_WGUI_TO_CLASS(WIMainMenuElement,WIMainMenuElement);

extern ClientState *client;
WIMainMenuBase::WIMainMenuBase()
	: WIBase(),m_selected(-1)
{
	AddStyleClass("main_menu");
}

void WIMainMenuBase::OnGoBack(int button,int action,int)
{
	if(button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
		return;
	WIMainMenu *mainMenu = dynamic_cast<WIMainMenu*>(GetParent());
	if(mainMenu == nullptr)
		return;
	mainMenu->OpenMainMenu();
}

void WIMainMenuBase::Initialize()
{
	WIBase::Initialize();
}
void WIMainMenuBase::MouseCallback(GLFW::MouseButton,GLFW::KeyState state,GLFW::Modifier)
{
	if(state != GLFW::KeyState::Press)
		return;
}
void WIMainMenuBase::KeyboardCallback(GLFW::Key key,int,GLFW::KeyState state,GLFW::Modifier)
{
	if(state != GLFW::KeyState::Press)
		return;
	if(key == GLFW::Key::S || key == GLFW::Key::Down) SelectNextItem();
	else if(key == GLFW::Key::W || key == GLFW::Key::Up) SelectPreviousItem();
	if(key == GLFW::Key::Enter)
	{
		WIMainMenuElement *el = GetSelectedElement();
		if(el == NULL)
			return;
		el->Activate();
	}
}
void WIMainMenuBase::SelectItem(int i)
{
	if(m_selected >= m_elements.size())
		m_selected = -1;
	if(m_selected == -1)
	{
		if(i >= m_elements.size())
			return;
		if(m_elements[i].IsValid() == false)
			return;
		m_selected = i;
		m_elements[i].get<WIMainMenuElement>()->Select();
		return;
	}
	if(m_elements[m_selected].IsValid())
		m_elements[m_selected].get<WIMainMenuElement>()->Deselect();
	if(i >= m_elements.size())
	{
		m_selected = -1;
		return;
	}
	m_selected = i;
	m_elements[i].get<WIMainMenuElement>()->Select();
}
WIMainMenuElement *WIMainMenuBase::GetSelectedElement()
{
	if(m_selected == -1 || m_selected >= m_elements.size())
		return NULL;
	auto &hElement = m_elements[m_selected];
	if(!hElement.IsValid())
		return NULL;
	return hElement.get<WIMainMenuElement>();
}
void WIMainMenuBase::SelectNextItem()
{
	if(m_selected == -1)
	{
		SelectItem(0);
		return;
	}
	SelectItem((m_selected < m_elements.size() -1) ? (m_selected +1) : 0);
}
void WIMainMenuBase::SelectPreviousItem()
{
	if(m_selected == -1)
	{
		SelectItem(CInt32(m_elements.size()) -1);
		return;
	}
	SelectItem((m_selected > 0) ? (m_selected -1) : (CInt32(m_elements.size()) -1));
}
void WIMainMenuBase::OnElementSelected(WIMainMenuElement *el)
{
	for(unsigned int i=0;i<m_elements.size();i++)
	{
		WIMainMenuElement *elCur = GetElement(i);
		if(elCur != NULL)
			if(elCur == el)
				m_selected = i;
			else
				elCur->Deselect();
	}
}
void WIMainMenuBase::UpdateElement(int i)
{
	WIMainMenuElement *el = GetElement(i);
	if(el == NULL)
		return;
	if(i == 0)
		el->SetPos(60,150);
	else
	{
		WIMainMenuElement *prev = GetElement(i -1);
		if(prev != NULL)
		{
			int yGap = 5;
			el->SetPos(60,prev->GetPos().y +prev->GetHeight() +yGap);
		}
	}
}
void WIMainMenuBase::UpdateElements()
{
	for(unsigned int i=0;i<m_elements.size();i++)
		UpdateElement(i);
}
void WIMainMenuBase::AddMenuItem(int pos,std::string name,const CallbackHandle &onActivated)
{
	auto hEl = CreateChild<WIMainMenuElement>();
	if(hEl.IsValid() == false)
		return;
	auto *el = dynamic_cast<WIMainMenuElement*>(hEl.get());
	el->SetText(name);
	el->SetSize(500,60);
	el->onActivated = onActivated;
	el->onSelected = FunctionCallback<void,WIMainMenuElement*>::Create([](WIMainMenuElement *el) {
		auto *parent = dynamic_cast<WIMainMenuBase*>(el->GetParent());
		if(parent == nullptr)
			return;
		parent->OnElementSelected(el);
	});
	el->AddStyleClass("main_menu_text");
	if(pos >= m_elements.size())
	{
		m_elements.push_back(el->GetHandle());
		UpdateElement(CInt32(m_elements.size()) -1);
	}
	else
	{
		m_elements.insert(m_elements.begin() +pos,el->GetHandle());
		UpdateElements();
	}
}
WIMainMenuElement *WIMainMenuBase::GetElement(int i)
{
	if(i >= m_elements.size())
		return NULL;
	auto &hElement = m_elements[i];
	if(!hElement.IsValid())
		return NULL;
	return hElement.get<WIMainMenuElement>();
}
void WIMainMenuBase::RemoveMenuItem(int i)
{
	if(i >= m_elements.size())
		return;
	auto &hEl = m_elements[i];
	if(hEl.IsValid())
		hEl->Remove();
	m_elements.erase(m_elements.begin() +i);
	UpdateElements();
}
void WIMainMenuBase::AddMenuItem(std::string name,const CallbackHandle &onActivated)
{
	AddMenuItem(CInt32(m_elements.size()),name,onActivated);
}
WIOptionsList *WIMainMenuBase::InitializeOptionsList()
{
	m_hControlSettings = CreateChild<WIOptionsList>();
	return m_hControlSettings.get<WIOptionsList>();
}
void WIMainMenuBase::InitializeOptionsList(WIOptionsList *pList)
{
	pList->SetPos(350,150);
	pList->SetWidth(600);
	pList->SizeToContents();
}

////////////////////////////

WIMainMenuElement::WIMainMenuElement()
	: WIBase(),onActivated(),onSelected(),onDeselected(),
	m_bSelected(false)
{
	RegisterCallback<void>("Select");
	RegisterCallback<void>("Deselect");
}

WIMainMenuElement::~WIMainMenuElement()
{}

void WIMainMenuElement::Select()
{
	if(m_bSelected == true)
		return;
	m_bSelected = true;
	
	/*if(m_hBackground.IsValid())
	{
		WITexturedRect *pRect = m_hBackground.get<WITexturedRect>();
		//pRect->SetVisible(true);
	}*/
	CallCallbacks<void>("Select");
	if(onSelected == nullptr)
		return;
	onSelected(this);
}

void WIMainMenuElement::Deselect()
{
	if(m_bSelected == false)
		return;
	m_bSelected = false;
	if(m_hBackground.IsValid())
	{
		WITexturedRect *pRect = m_hBackground.get<WITexturedRect>();
		pRect->SetVisible(false);
	}
	CallCallbacks<void>("Deselect");
	if(onDeselected == nullptr)
		return;
	onDeselected(this);
}

void WIMainMenuElement::Initialize()
{
	WIBase::Initialize();
	//WITexturedRect *pBackground = WGUI::Create<WITexturedRect>(this);
	//UNUSED(pBackground);
	/*if(pBackground != NULL)
	{
		m_hBackground = pBackground->GetHandle();
		pBackground->SetMaterial("wgui/menu_item_selected");
		pBackground->SetVisible(false);
	}*/
	WIText *pText = WGUI::GetInstance().Create<WIText>(this);
	if(pText != NULL)
	{
		m_hText = pText->GetHandle();
		/*pText->SetFont("MainMenu_Regular");
		pText->SetColor(MENU_ITEM_COLOR);
		
		pText->EnableShadow(true);
		pText->SetShadowColor(MENU_ITEM_SHADOW_COLOR);
		pText->SetShadowOffset(MENU_ITEM_SHADOW_OFFSET);*/
	}
	SetMouseInputEnabled(true);
}

void WIMainMenuElement::Activate()
{
	if(onActivated == nullptr)
		return;
	onActivated(this);
}

void WIMainMenuElement::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	WIBase::MouseCallback(button,state,mods);
	if(state != GLFW::KeyState::Press)
		return;
	Activate();
}

void WIMainMenuElement::OnCursorEntered() {Select();}
void WIMainMenuElement::OnCursorExited() {Deselect();}

void WIMainMenuElement::SetText(std::string &text)
{
	if(m_hText.IsValid())
	{
		WIText *pText = m_hText.get<WIText>();
		pText->SetText(text);
		pText->SizeToContents();
	}
}
void WIMainMenuElement::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	if(m_hBackground.IsValid())
	{
		WITexturedRect *pRect = m_hBackground.get<WITexturedRect>();
		pRect->SetSize(x,y);
	}
	if(m_hText.IsValid())
	{
		WIText *pText = m_hText.get<WIText>();
		pText->SetPos(44,CInt32(y *0.5f -pText->GetHeight() *0.45f));
	}
}
Vector4 WIMainMenuElement::GetBackgroundColor()
{
	if(!m_hBackground.IsValid())
		return Vector4(1,1,1,1);
	return m_hBackground->GetColor().ToVector4();
}
void WIMainMenuElement::SetBackgroundColor(float r,float g,float b,float a)
{
	if(!m_hBackground.IsValid())
		return;
	m_hBackground->SetColor(r,g,b,a);
}