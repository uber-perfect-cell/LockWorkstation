/*
 * Copyright 2024, cafeina <cafeina@world>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <Catalog.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <stdlib.h>
#include "mLoginBox.h"
#include "View.h"
#include "Box.h"
#include "mConstant.h"



#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Login box"

static BString strLoginFailed = B_TRANSLATE("Login failed.");
static BString strLoginEaster = B_TRANSLATE("fruitless attempts made\ntime spent which will not return\na shame, won't you say"); // thought it would be a fun addition
static BString strPassExpired = B_TRANSLATE("The password is expired.");
static BString strNoError = "";
int loginAttempts;

mLoginBox::mLoginBox(BRect frame)
: BView(frame, "v_loginbox", B_FOLLOW_LEFT, B_WILL_DRAW | B_NAVIGABLE |
    B_NAVIGABLE_JUMP | B_INPUT_METHOD_AWARE | B_DRAW_ON_CHILDREN)
{
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

    BFont font1(be_plain_font);
    font1.SetSize(be_plain_font->Size() * 2);
    BStringView* descriptionView = new BStringView("sv_desc",
        B_TRANSLATE("Session locked"));
    descriptionView->SetFont(&font1);

    BFont font2(be_plain_font);
    font2.SetSize(be_plain_font->Size() * 1.5);
    font2.SetFace(B_ITALIC_FACE);

    BStringView* instructionsView = new BStringView("sv_inst",
        B_TRANSLATE("Write your username and password to unlock:"));
    instructionsView->SetFont(&font2);

    BFont font3;
    font3.SetSize(be_plain_font->Size() * (1.5));

    tcUserName = new BTextControl("tc_username", B_TRANSLATE("Username"), "",
        new BMessage('user'));
    tcUserName->SetModificationMessage(new BMessage(LBM_USERNAME_CHANGED));
    tcUserName->SetFont(&font3);
    tcUserName->MakeFocus(true);
    tcPassword = new BTextControl("tc_password", B_TRANSLATE("Password"), "",
        new BMessage('pass'));
    tcPassword->SetModificationMessage(new BMessage(LBM_PASSWORD_CHANGED));
    tcPassword->SetFont(&font3);
    tcPassword->TextView()->HideTyping(true);
    btLogin = new BButton("bt_login", B_TRANSLATE("Login"),
        new BMessage(LBM_LOGIN_REQUESTED));
    btLogin->SetFont(&font3);
    btLogin->SetEnabled(IsAbleToLogin());
    btLogin->MakeDefault(true);

    BFont errorFont(be_bold_font);
    errorFont.SetSize(be_bold_font->Size() * 1.5);

    errorView = new BStringView("sv_error", strNoError);
    errorView->SetFont(&errorFont);
    errorView->SetHighColor(ui_color(B_FAILURE_COLOR));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0 | B_PLAIN_BORDER)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(descriptionView)
        .Add(instructionsView)
        .AddStrut(16.0f)
        .AddGrid()
            .AddTextControl(tcUserName, 0, 0)
            .AddTextControl(tcPassword, 0, 1)
        .End()
        .AddStrut(12.0f)
        .AddGroup(B_HORIZONTAL)
            .Add(errorView)
            .AddGlue()
            .Add(btLogin)
            .SetInsets(0, 0, 0, 0)
            .End()
    .End();

    ResizeToPreferred();
}

mLoginBox::~mLoginBox()
{
}

// #pragma mark -

void mLoginBox::AttachedToWindow()
{
    tcUserName->SetTarget(this);
    tcPassword->SetTarget(this);
    btLogin->SetTarget(this);
}

void mLoginBox::MessageReceived(BMessage* message)
{
    switch(message->what)
    {
        case 'user':
        case loginBoxMsgs::LBM_USERNAME_CHANGED:
            ThreadedCall(thUpdateUIForm, CallUpdateUIForm,
                "Update error message", B_NORMAL_PRIORITY, this);
            break;
        case 'pass':
        case loginBoxMsgs::LBM_PASSWORD_CHANGED:
            ThreadedCall(thUpdateUIForm, CallUpdateUIForm,
                "Update error message", B_NORMAL_PRIORITY, this);
            break;
        case loginBoxMsgs::LBM_LOGIN_REQUESTED:
            if(IsAbleToLogin())
                RequestLogin();
            break;
        case M_LOGIN_FAILED:
            fprintf(stderr, "Login failed.\n");
            ThreadedCall(thUpdateUIErrorMsg, CallUpdateUIErrorMsg,
                "Update error message", B_NORMAL_PRIORITY, this);
            ++loginAttempts;
            break;
        case M_PASSWORD_EXPIRED:
            fprintf(stderr, "Password is expired.\n");
            ThreadedCall(thUpdateUIErrorMsg, CallUpdateUIExpiredMsg,
                "Update error message", B_NORMAL_PRIORITY, this);
            break;
        default:
            BView::MessageReceived(message);
            break;
    }
    if (loginAttempts == 100) {
        ThreadedCall(thUpdateUIErrorMsg, CallUpdateUIEasterMsg,
        "Update error message", B_NORMAL_PRIORITY, this);
    }
}

void mLoginBox::Draw(BRect updateRect)
{
    BView::Draw(updateRect);
    SetHighColor(0, 0, 0, 255);
    StrokeRect(Bounds());
    Invalidate();
}

void mLoginBox::RequestLogin()
{
    BMessage message(BUTTON_LOGIN);
    message.AddString("username", tcUserName->Text());
    message.AddString("password", tcPassword->Text());
    Window()->PostMessage(&message, Window(), this);
}

// #pragma mark -

bool mLoginBox::IsAbleToLogin()
{
    return tcUserName->TextLength() > 0 && tcPassword->TextLength() > 0;
}

int mLoginBox::CallUpdateUIForm(void* data)
{
    mLoginBox* box = (mLoginBox*)data;
    box->UpdateUIForm();
    return 0;
}

int mLoginBox::CallUpdateUIEasterMsg(void* data)
{
    mLoginBox* box = (mLoginBox*)data;
    box->UpdateUIErrorMsg(strLoginEaster);
    return 0;
}


int mLoginBox::CallUpdateUIErrorMsg(void* data)
{
    mLoginBox* box = (mLoginBox*)data;
    box->UpdateUIErrorMsg(strLoginFailed);
    return 0;
}



int mLoginBox::CallUpdateUIExpiredMsg(void *data)
{
    mLoginBox* box = (mLoginBox*)data;
    box->UpdateUIErrorMsg(strPassExpired);
    return 0;
}

void mLoginBox::UpdateUIForm()
{
    LockLooper();
    tcUserName->MarkAsInvalid(!(tcUserName->TextLength() > 0));
    tcPassword->MarkAsInvalid(!(tcPassword->TextLength() > 0));
    btLogin->SetEnabled(IsAbleToLogin());
    UnlockLooper();
}

void mLoginBox::UpdateUIErrorMsg(BString whatstr)
{
    LockLooper();
    errorView->SetText(whatstr);
    UnlockLooper();
}
