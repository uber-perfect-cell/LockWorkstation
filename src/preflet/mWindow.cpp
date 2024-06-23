/**********************************************************************************/
/*****************        Lock Workstation By Marius Stene        *****************/
/*****************             m_stene@yahoo.com, 2002            *****************/
/*****************        My contribution to this great OS        *****************/
/**********************************************************************************/
/*****************  You are welcome to use any part of this code  *****************/
/*****************      in whole, or in parts freely without      *****************/
/*****************                  contacting me                 *****************/
/**********************************************************************************/
//Global
#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Spinner.h>
#include <iostream>
#include <stdio.h>
//Local
#include "mWindow.h"
#include "mApp.h"
#include "../common/LockWorkstationConfig.h"

const BRect mWindowRect 					(64, 64, 504, 424);

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "mWindow"

//Constructa
mWindow::mWindow(const char *mWindowTitle)
: BWindow(BRect(mWindowRect), mWindowTitle, B_TITLED_WINDOW,
    B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_RESIZABLE)
{
    InitUIData(); // Load data from config file

    amthdCardView = CreateCardView_AccountMethod();
    userCardView  = CreateCardView_User();
	bgCardView    = CreateCardView_Background();
	clockCardView = CreateCardView_Clock();
	extraCardView = CreateCardView_Options();

    //I te yo evythin
    mButtonApplyEverything = new BButton("mFrameButtonApplyEverything",
        B_TRANSLATE("Apply"), new BMessage(APPLY_EVERYTHING), B_WILL_DRAW | B_NAVIGABLE);
    mEraserButtonOfDoom = new BButton("EraserButtonOfDoom",
        B_TRANSLATE("Default"), new BMessage(ERASER_FROM_DOOM));

    mApplyView = new BView(BRect(0, 322, 500, 360), "ApplyView", B_FOLLOW_ALL_SIDES,
        B_WILL_DRAW | B_NAVIGABLE);
    mApplyView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    mApplyView->SetHighUIColor(B_PANEL_TEXT_COLOR);

    BLayoutBuilder::Group<>(mApplyView, B_HORIZONTAL)
        .AddGlue()
        .Add(mEraserButtonOfDoom)
        .Add(mButtonApplyEverything)
    .End();

    // Main view
    fPanelList = new BListView(B_SINGLE_SELECTION_LIST);
    fPanelList->SetSelectionMessage(new BMessage(M_ITEM_SELECTED));
    fPanelList->AddItem(new BStringItem(B_TRANSLATE("Authentication method")));
    fPanelList->AddItem(new BStringItem(B_TRANSLATE("Application user")));
    fPanelList->AddItem(new BStringItem(B_TRANSLATE("Background")));
    fPanelList->AddItem(new BStringItem(B_TRANSLATE("Clock")));
    fPanelList->AddItem(new BStringItem(B_TRANSLATE("Options")));
    BScrollView *listScrollView = new BScrollView("sc_cont", fPanelList,
        0, false, true, B_FANCY_BORDER);
    fPanelList->SetExplicitMinSize(
        BSize(fPanelList->StringWidth(B_TRANSLATE("Background")) * 1.5,
        B_SIZE_UNSET));

    fCardView = new BCardView();
    fCardView->AddChild(amthdCardView);
    fCardView->AddChild(userCardView);
    fCardView->AddChild(bgCardView);
    fCardView->AddChild(clockCardView);
    fCardView->AddChild(extraCardView);

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        .AddGroup(B_HORIZONTAL)
            .Add(listScrollView)
            .Add(fCardView)
        .End()
        .Add(mApplyView)
    .End();

    InitUIControls(); // Set controls' values to the data values

    BMessenger msgr(this);
    be_roster->StartWatching(msgr, B_SOME_APP_ACTIVATED);

    mFilePanelFolderBrowse = new BFilePanel(B_OPEN_PANEL, &msgr, &mEntryRef,
        B_DIRECTORY_NODE, false, new BMessage(FOLDER_CHANGED), NULL, true, true);

    AddShortcut('A', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
    AddShortcut('S', B_COMMAND_KEY, new BMessage(APPLY_EVERYTHING));
    AddShortcut('D', B_COMMAND_KEY, new BMessage(ERASER_FROM_DOOM));

    //Enable and disable buttons
    ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
        "Enable and disable buttons", B_LOW_PRIORITY, this);

    // ResizeToPreferred();
    CenterOnScreen();
}

mWindow::~mWindow()
{
    delete settings;
}

void mWindow::MessageReceived(BMessage* message)
{
    switch(message->what)
	{
        case M_ITEM_SELECTED:
        {
            int32 index = message->GetInt32("index", 0);
            if (index >= 0 && index < fCardView->CountChildren())
				fCardView->CardLayout()->SetVisibleItem(index);
            break;
        }
        case M_AUTHMTHD_SYSTEM:
        {
            settings->SetAuthenticationMethod(AUTH_SYSTEM_ACCOUNT);

            //Enable and disable buttons
            ThreadedCall(EnDUserButtonThread, EnDUserButtonThread_static,
                "Enable and disable user button", B_LOW_PRIORITY, this);
            break;
        }
        case M_AUTHMTHD_KEYSTR:
        {
            settings->SetAuthenticationMethod(AUTH_KEYSTORE);

            //Enable and disable buttons
            ThreadedCall(EnDUserButtonThread, EnDUserButtonThread_static,
                "Enable and disable user button", B_LOW_PRIORITY, this);
            break;
        }
        case M_AUTHMTHD_APPACC:
        {
            settings->SetAuthenticationMethod(AUTH_APP_ACCOUNT);

            //Enable and disable buttons
            ThreadedCall(EnDUserButtonThread, EnDUserButtonThread_static,
                "Enable and disable user button", B_LOW_PRIORITY, this);
            break;
        }
        case BUTTON_DEFAULTPATH:
        {
            BMessage* temp = new BMessage;
            LWSettings::DefaultSettings(temp);
            settings->SetBackgroundImageFolderPath(temp->GetString(mNameConfigImagePath));
            mTextControlmPathToImageFolder->SetText(settings->BackgroundImageFolderPath());
            delete temp;

            //Check if there are any images there!
            ThreadedCall(CheckerThread, CheckerThread_static,
                "Check if images are there thread", B_LOW_PRIORITY, this);

            //Update Strings
            ThreadedCall(UpdateStringsThread, UpdateStringsThread_static,
                "Update Strings", B_NORMAL_PRIORITY, this);

            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        /*****************************************************************************/
        /******************************** DIE!!! *************************************/
        /*****************************************************************************/
        case ERASER_FROM_DOOM:
        {
            settings->Reset();
            settings->SaveSettings();
            InitUIControls();

            //Check if images are there
            ThreadedCall(CheckerThread, CheckerThread_static,
                "Check if images are there thread", B_LOW_PRIORITY, this);
            /*buttons*/
            mApplyView->LockLooper();
            mButtonApplyEverything->SetEnabled(false);
            mEraserButtonOfDoom->SetEnabled(false);
            mApplyView->UnlockLooper();
            //Bool clock
            // mCheckBoxBoolClock->SetValue(atoi(mDefaultBoolClock));
            //slider
            // mStringClockFontSize.SetTo("");
            // mStringClockFontSize << mSliderFontSize->Value();
            //Update Strings
            ThreadedCall(UpdateStringsThread, UpdateStringsThread_static,
                "Update Strings", B_NORMAL_PRIORITY, this);
            break;
        }
        /********************************************************************/
        /************************** LIVE! ***********************************/
        /********************************************************************/
        case APPLY_EVERYTHING:
        {
            settings->Commit();
            settings->SaveSettings();

            //Check if there are any images there!
            ThreadedCall(CheckerThread, CheckerThread_static,
                "Check if images are there thread", B_NORMAL_PRIORITY, this);

            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case BOOL_CLOCK:
        {
            settings->SetClockEnabled(mCheckBoxBoolClock->Value() == B_CONTROL_ON);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case M_BOOL_SESSION:
        {
            settings->SetSessionBarEnabled(mCheckBoxSessionBar->Value() == B_CONTROL_ON);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case M_BOOL_INFO:
        {
            settings->SetSystemInfoPanelEnabled(mCheckBoxSysInfo->Value() == B_CONTROL_ON);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case M_BOOL_KILLER:
        {
            settings->SetKillerShortcutEnabled(mCheckBoxKillerShortcut->Value() == B_CONTROL_ON);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case M_BOOL_EVTLOG:
        {
            settings->SetEventLogEnabled(mCheckBoxEventLog->Value() == B_CONTROL_ON);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case COLOR_CHANGED_R:
        case COLOR_CHANGED_G:
        case COLOR_CHANGED_B:
        {
            settings->SetBackgroundColor({
                static_cast<uint8>(mSpinnerColorR->Value()),
                static_cast<uint8>(mSpinnerColorG->Value()),
                static_cast<uint8>(mSpinnerColorB->Value())
            });
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case CLOCKCOLOR_CHANGED_R:
        case CLOCKCOLOR_CHANGED_G:
        case CLOCKCOLOR_CHANGED_B:
        {
            settings->SetClockColor({
                static_cast<uint8>(mSpinnerClockColorR->Value()),
                static_cast<uint8>(mSpinnerClockColorG->Value()),
                static_cast<uint8>(mSpinnerClockColorB->Value())
            });
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case DEFAULT_COLORS:
        {
            BMessage* temp = new BMessage;
            LWSettings::DefaultSettings(temp);

            settings->SetBackgroundColor(temp->GetColor(mNameConfigBgColor, {}));
            mSpinnerColorR->SetValue(settings->BackgroundColor().red);
            mSpinnerColorG->SetValue(settings->BackgroundColor().green);
            mSpinnerColorB->SetValue(settings->BackgroundColor().blue);

            //Update Strings
            ThreadedCall(UpdateStringsThread, UpdateStringsThread_static,
                "Update Strings", B_NORMAL_PRIORITY, this);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);

            delete temp;
            break;
        }
        case DEFAULT_CLOCK_COLORS:
        {
            BMessage* temp = new BMessage;
            LWSettings::DefaultSettings(temp);

            settings->SetClockColor(temp->GetColor(mNameConfigClockColor, {}));
            mSpinnerClockColorR->SetValue(settings->ClockColor().red);
            mSpinnerClockColorG->SetValue(settings->ClockColor().green);
            mSpinnerClockColorB->SetValue(settings->ClockColor().blue);

            //Update Strings
            ThreadedCall(UpdateStringsThread, UpdateStringsThread_static,
                "Update Strings", B_NORMAL_PRIORITY, this);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);

            delete temp;
            break;
        }
        case CLOCKPLACEMENT_X:
        {
            BScreen screen(B_MAIN_SCREEN_ID);
            float x = atof(mTextControlClockPlaceX->Text());

            if(x >= 0 && x < screen.Frame().Width())
                settings->SetClockLocation(BPoint(x, settings->ClockLocation().y));
            else
                settings->SetClockLocation(BPoint(0, settings->ClockLocation().y));

            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case CLOCKPLACEMENT_Y:
        {
            BScreen screen(B_MAIN_SCREEN_ID);
            float y = atof(mTextControlClockPlaceY->Text());

            if(y >= 0 && y < screen.Frame().Height())
                settings->SetClockLocation(BPoint(settings->ClockLocation().x, y));
            else
                settings->SetClockLocation(BPoint(settings->ClockLocation().x, 0));

            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case DEFAULT_PLACE:
        {
            BMessage* temp = new BMessage;
            LWSettings::DefaultSettings(temp);

            settings->SetClockLocation(temp->GetPoint(mNameConfigClockPlace, BPoint()));
            mTextControlClockPlaceX->SetText(std::to_string(settings->ClockLocation().x).c_str());
            mTextControlClockPlaceY->SetText(std::to_string(settings->ClockLocation().y).c_str());

            //Bool clock
            mCheckBoxBoolClock->SetValue(settings->ClockIsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
            //Update Strings
            ThreadedCall(UpdateStringsThread, UpdateStringsThread_static,
                "Update Strings", B_NORMAL_PRIORITY, this);
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);

            delete temp;
            break;
        }
        case SIZE_SLIDER_CHANGED:
        {
            settings->SetClockSize(mSliderFontSize->Value());

            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case M_SNOOZE_SLIDER_CHANGED:
        {
            settings->SetBackgroundImageSnooze(mSliderBgSnooze->Value());

            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case CHANGE_LOGIN:
        {
            settings->SetDefaultUser(mAddUserName->Text());
        	settings->SetDefaultUserPassword(mAddPassWord->Text());

            // AppUserMod(mAddUserName->Text(), mAddPassWord->Text(), NULL);

        	//Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        }
        case M_APPUSER_SEL:
        {
            // mButtonUserMod->SetEnabled(true);
            // mButtonUserRem->SetEnabled(true);
            break;
        }
        case M_APPUSER_IVK:
        {
            // mButtonUserMod->SetEnabled(true);
            // mButtonUserRem->SetEnabled(true);

            // int32 pos = mListOfUsers->CurrentSelection();
            // mAddUserName->SetText(((BStringItem*)mListOfUsers->ItemAt(pos))->Text());
            break;
        }
        case M_APPUSER_MODIFY:
        {
            // int32 pos = mListOfUsers->CurrentSelection();
            // mAddUserName->SetText(((BStringItem*)mListOfUsers->ItemAt(pos))->Text());
            break;
        }
        case M_APPUSER_REMOVE:
        {
            // int32 pos = mListOfUsers->CurrentSelection();
            // BStringItem* targetitem = (BStringItem*)mListOfUsers->ItemAt(pos);
//
            // if(targetitem != NULL) {
                // const char* del = targetitem->Text();
                // if(settings->RemoveAppUser(del) == B_OK) {
                    // mListOfUsers->RemoveItem(targetitem);
                    // mButtonUserMod->SetEnabled(false);
                    // mButtonUserRem->SetEnabled(false);
                // }
                // else
                    // fprintf(stderr, "User could not be deleted. Either it did not"
                    // " exist or it was not allowed to delete it.\n");
            // }
            break;
        }
        case BROWSE_FOLDER:
            mFilePanelFolderBrowse->Show();

            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        case FOLDER_CHANGED:
        {
            if(message->FindRef("refs", &mEntryRef) == B_OK) {
                BEntry entry(&mEntryRef);
                BPath path;
                entry.GetPath(&path);
                settings->SetBackgroundImageFolderPath(path.Path());
                mTextControlmPathToImageFolder->SetText(settings->BackgroundImageFolderPath());
            }

            ThreadedCall(CheckerThread, CheckerThread_static,
                "Check if images are there thread", B_NORMAL_PRIORITY, this);
            break;
        }
        case CHECK_BUTTONS:
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            break;
        case CHECK_USERBUTTON:
            ThreadedCall(EnDUserButtonThread, EnDUserButtonThread_static,
                "Enable and disable user button", B_LOW_PRIORITY, this);
            break;
        case M_BGMODE_RADIO_NONE:
            settings->SetBackgroundMode(BGM_NONE);

            //Enable and disable buttons
            ThreadedCall(EnDUserButtonThread, EnDUserButtonThread_static,
                "Enable and disable user button", B_LOW_PRIORITY, this);
            break;
        case M_BGMODE_RADIO_FOLDER:
            settings->SetBackgroundMode(BGM_FOLDER);

            //Enable and disable buttons
            ThreadedCall(EnDUserButtonThread, EnDUserButtonThread_static,
                "Enable and disable user button", B_LOW_PRIORITY, this);
            break;
        case M_BGMODE_RADIO_LIST:
            settings->SetBackgroundMode(BGM_LISTFILE);

            //Enable and disable buttons
            ThreadedCall(EnDUserButtonThread, EnDUserButtonThread_static,
                "Enable and disable user button", B_LOW_PRIORITY, this);
            break;
        case B_ABOUT_REQUESTED:
            be_app->PostMessage(B_ABOUT_REQUESTED);
            break;
        default:
            //Enable and disable buttons
            ThreadedCall(EnDButtonsThread, EnDButtonsThread_static,
                "Enable and disable buttons", B_LOW_PRIORITY, this);
            BWindow::MessageReceived(message);
            break;
	}
}

bool mWindow::QuitRequested()
{
    be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}

void mWindow::AppUserMod(const char* n, const char* p, const char* nn)
{
    // bool found = false;
    // int i = 0;

    // while(!found && i < settings->AppUserList().CountStrings()) {
        // if(strcmp(settings->AppUserList().StringAt(i).String(), n) == 0)
            // found = true;
        // else
            // i++;
    // }

    // if(found) { // User exists, we must modify it
        // settings->ChangeAppUserPassword(n, p);
    // }
    // else {  // User does not exist, we have to create it
        // if(settings->AddAppUser(n, p) != B_OK)
            // fprintf(stderr, "could not be added\n");
    // } // Currently we won't deal with the cases of username change
}

// #pragma mark -

int32 mWindow::CheckerThread_static(void *data)
{
    mWindow *changeimage = (mWindow *)data;
	changeimage->Checker_Thread();
	return 0;
}

void mWindow::Checker_Thread()
{
}

int32
mWindow::EnDButtonsThread_static(void *data)
{
	mWindow *enabuttons = (mWindow *)data;
	enabuttons->EnDButtons_Thread();
	return 0;
}

void mWindow::EnDButtons_Thread()
{
    snooze(10000);

    BMessage* defaults = new BMessage;
    LWSettings::DefaultSettings(defaults);

    bgCardView->LockLooper();
    mButtonDefaultColors->SetEnabled(!UI_IsBgColorDefault(defaults));
    mButtonDefaultImagePath->SetEnabled(!UI_IsBgFolderDefault(defaults));
    bgCardView->UnlockLooper();

    clockCardView->LockLooper();
    mButtonDefaultClockColors->SetEnabled(!UI_IsClockColorDefault(defaults));
    mButtonDefaultClockPlace->SetEnabled(!UI_IsClockPlaceDefault(defaults));
    clockCardView->UnlockLooper();

    /* Reset everything to defaults */
    mApplyView->LockLooper();
    mEraserButtonOfDoom->SetEnabled(!UI_IsDefault(defaults));
    mApplyView->UnlockLooper();

    delete defaults;
}

/*******Enable and Disable User Button**************/
int32
mWindow::EnDUserButtonThread_static(void *data)
{
	mWindow *enauserbutton = (mWindow *)data;
	enauserbutton->EnDUserButton_Thread();
	return 0;
}

/*************************************************/
void mWindow::EnDUserButton_Thread()
{
    bool validUser = strcmp(mAddUserName->Text(), "") != 0;
    bool verifiedPwd = strcmp(mAddPassWord->Text(), mAddPassWordRetype->Text()) == 0 &&
                       strcmp(mAddPassWord->Text(), "") != 0;

    userCardView->LockLooper();
    mButtonChangeLogin->SetEnabled(validUser && verifiedPwd);
    userCardView->UnlockLooper();
}

int32
mWindow::UpdateStringsThread_static(void *data)
{
	mWindow *updatestrings = (mWindow *)data;
	updatestrings->UpdateStrings_Thread();
	return 0;
}

void mWindow::UpdateStrings_Thread()
{
    LockLooper();

    mAddUserName->SetText(settings->DefaultUser());
    ((BStringView*)mListOfUsers->ItemAt(0))->SetText(settings->DefaultUser());

    settings->BackgroundColor().set_to(
        static_cast<uint8>(mSpinnerColorR->Value()),
        static_cast<uint8>(mSpinnerColorG->Value()),
        static_cast<uint8>(mSpinnerColorB->Value())
    );
	settings->SetBackgroundImageFolderPath(mTextControlmPathToImageFolder->Text());

    settings->SetClockEnabled(mCheckBoxBoolClock->Value() == B_CONTROL_ON);
    settings->SetClockSize((uint32)mSliderFontSize->Value());
    settings->ClockColor().set_to(
        static_cast<uint8>(mSpinnerClockColorR->Value()),
        static_cast<uint8>(mSpinnerClockColorG->Value()),
        static_cast<uint8>(mSpinnerClockColorB->Value())
    );
    settings->ClockLocation().Set(
        static_cast<float>(atof(mTextControlClockPlaceX->Text())),
        static_cast<float>(atof(mTextControlClockPlaceY->Text()))
    );

    settings->SetSessionBarEnabled(mCheckBoxSessionBar->Value() == B_CONTROL_ON);
    settings->SetSystemInfoPanelEnabled(mCheckBoxSysInfo->Value() == B_CONTROL_ON);
    settings->SetKillerShortcutEnabled(mCheckBoxKillerShortcut->Value() == B_CONTROL_ON);
    settings->SetEventLogEnabled(mCheckBoxEventLog->Value() == B_CONTROL_ON);

    UnlockLooper();
}

void mWindow::InitUIControls()
{
    LockLooper();

    switch(settings->AuthenticationMethod()) {
        case 0:
            mRadioBtAuthSysaccount->SetValue(B_CONTROL_ON);
            mRadioBtAuthSyskeystore->SetValue(B_CONTROL_OFF);
            mRadioBtAuthAppaccount->SetValue(B_CONTROL_OFF);
            break;
        case 2:
            mRadioBtAuthSysaccount->SetValue(B_CONTROL_OFF);
            mRadioBtAuthSyskeystore->SetValue(B_CONTROL_OFF);
            mRadioBtAuthAppaccount->SetValue(B_CONTROL_ON);
            break;
        case 1:
        default:
            mRadioBtAuthSysaccount->SetValue(B_CONTROL_OFF);
            mRadioBtAuthSyskeystore->SetValue(B_CONTROL_ON);
            mRadioBtAuthAppaccount->SetValue(B_CONTROL_OFF);
            break;
    }
    mAddUserName->SetText(settings->DefaultUser());
    mListOfUsers->AddItem(new BStringItem(settings->DefaultUser()));
    // for(int i = 0; i < settings->AppUserList().CountStrings(); i++) {
        // fprintf(stderr, "Found: %s\n", settings->AppUserList().StringAt(i).String());
        // mListOfUsers->AddItem(new BStringItem(settings->AppUserList().StringAt(i).String()));
    // }
    mSpinnerColorR->SetValue(settings->BackgroundColor().red);
    mSpinnerColorG->SetValue(settings->BackgroundColor().green);
    mSpinnerColorB->SetValue(settings->BackgroundColor().blue);
    mTextControlmPathToImageFolder->SetText(settings->BackgroundImageFolderPath());
    mSliderBgSnooze->SetValue(settings->BackgroundImageSnooze());
    mCheckBoxBoolClock->SetValue(settings->ClockIsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
    mSliderFontSize->SetValue(settings->ClockSize());
    mSpinnerClockColorR->SetValue(settings->ClockColor().red);
    mSpinnerClockColorG->SetValue(settings->ClockColor().green);
    mSpinnerClockColorB->SetValue(settings->ClockColor().blue);
    mTextControlClockPlaceX->SetText(std::to_string(settings->ClockLocation().x).c_str());
    mTextControlClockPlaceY->SetText(std::to_string(settings->ClockLocation().y).c_str());
    mCheckBoxSessionBar->SetValue(settings->SessionBarIsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
    mCheckBoxSysInfo->SetValue(settings->SystemInfoPanelIsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
    mCheckBoxKillerShortcut->SetValue(settings->KillerShortcutIsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
    mCheckBoxEventLog->SetValue(settings->EventLogIsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
    switch(settings->BackgroundMode()) {
        case 0:
            mRadioBtUseBgImgNone->SetValue(B_CONTROL_ON);
            mRadioBtUseBgImgFolder->SetValue(B_CONTROL_OFF);
            mRadioBtUseBgImgList->SetValue(B_CONTROL_OFF);
            break;
        case 2:
            mRadioBtUseBgImgNone->SetValue(B_CONTROL_OFF);
            mRadioBtUseBgImgFolder->SetValue(B_CONTROL_OFF);
            mRadioBtUseBgImgList->SetValue(B_CONTROL_ON);
            break;
        case 1:
        default:
            mRadioBtUseBgImgNone->SetValue(B_CONTROL_OFF);
            mRadioBtUseBgImgFolder->SetValue(B_CONTROL_ON);
            mRadioBtUseBgImgList->SetValue(B_CONTROL_OFF);
            break;
    }

    UnlockLooper();
}

void mWindow::InitUIData()
{
    BPath path;
    find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(mPathToConfigFile);

    settings = new LWSettings(path.Path());
}

// #pragma mark - UI Building

BView* mWindow::CreateCardView_AccountMethod()
{
    mRadioBtAuthSysaccount = new BRadioButton("rb_sysacc",
        B_TRANSLATE("Authenticate using a system account"),
        new BMessage(M_AUTHMTHD_SYSTEM));
    mRadioBtAuthSysaccount->SetFont(be_bold_font);
    BStringView* sysaccountDesc = new BStringView(NULL,
        B_TRANSLATE_COMMENT(
            "This authentication method requires an existing user account\n"
            "in the system to work. The user account must not be a \n"
            "service account. This application allows to login with accounts \n"
            "having an empty password.", "Please place the new line characters"
            "accordingly to accomodate the string, in order to not stretch "
            "the window too much")
    );

    mRadioBtAuthSyskeystore = new BRadioButton("rb_syskey",
        B_TRANSLATE("Authenticate using a key from the Keystore"),
        new BMessage(M_AUTHMTHD_KEYSTR));
    mRadioBtAuthSyskeystore->SetFont(be_bold_font);
    BStringView* keyaccountDesc = new BStringView(NULL,
        B_TRANSLATE_COMMENT(
            "This authentication method makes use of the keystore server, \n"
            "from where it retrieves the username-password pair, stored in \n"
            "this application's keyring (LockWorkstation) in the system. The \n"
            "first time this option is enabled from this preflet, it will ask\n"
            "the system to create such keyring in the keystore, and later it\n"
            "will also ask for permission to add a key containing a pair of \n"
            "username-password.", "Please place the new line characters"
            "accordingly to accomodate the string, in order to not stretch "
            "the window too much")
    );

    mRadioBtAuthAppaccount = new BRadioButton("rb_appacc",
        B_TRANSLATE("Authenticate using an application based account (default)"),
        new BMessage(M_AUTHMTHD_APPACC));
    mRadioBtAuthAppaccount->SetFont(be_bold_font);
    BStringView* appaccountDesc = new BStringView(NULL,
        B_TRANSLATE_COMMENT(
            "This authentication method will make use of the username-password\n"
            "pairs saved in this application's settings file. To manage the \n"
            "application accounts, please make use of this application's view\n"
            "\"Application user\". Please consider that deleting or restoring \n"
            "the default values may delete such application-based accounts.",
            "Please place the new line characters accordingly to accomodate the"
            " string, in order to not stretch the window too much")
    );

    BView* thisview = new BView("v_acc_mthd", B_SUPPORTS_LAYOUT, NULL);
    BLayoutBuilder::Group<>(thisview, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        .Add(mRadioBtAuthSysaccount)
        .Add(sysaccountDesc)
        // .Add(mRadioBtAuthSyskeystore)
        // .Add(keyaccountDesc)
        .Add(mRadioBtAuthAppaccount)
        .Add(appaccountDesc)
        .AddGlue()
    .End();

    mRadioBtAuthSyskeystore->SetEnabled(false);

    return thisview;
}

BView* mWindow::CreateCardView_User()
{
    /* AddUser */
    mAddUserName = new BTextControl("TextAddUser", B_TRANSLATE("Username"),
        "", new BMessage(TEXTADDUSER));
    mAddUserName->SetModificationMessage(new BMessage(CHECK_USERBUTTON));
    mAddPassWord = new BTextControl("TextPassWord", B_TRANSLATE("Password"),
        "", new BMessage(TEXTADDPASS));
    mAddPassWord->SetModificationMessage(new BMessage(CHECK_USERBUTTON));
    mAddPassWord->TextView()->HideTyping(true);
    mAddPassWordRetype = new BTextControl("TextPassWord", B_TRANSLATE("Repeat"),
        "", new BMessage(TEXTADDPASS));
    mAddPassWordRetype->SetModificationMessage(new BMessage(CHECK_USERBUTTON));
    mAddPassWordRetype->TextView()->HideTyping(true);
    mButtonChangeLogin = new BButton("LoginChange", B_TRANSLATE("Change"),
        new BMessage(CHANGE_LOGIN));
    mButtonChangeLogin->SetEnabled(false);

    BView* mView = new BView(BRect(0, 0, 235, 120), "MainView", B_FOLLOW_ALL_SIDES,
        B_WILL_DRAW | B_NAVIGABLE);
    mView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    mView->SetHighUIColor(B_PANEL_TEXT_COLOR);

    BLayoutBuilder::Group<>(mView, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        .AddGrid()
            .AddTextControl(mAddUserName, 0, 0)
            .AddTextControl(mAddPassWord, 0, 1)
            .AddTextControl(mAddPassWordRetype, 0, 2)
        .End()
        .Add(mButtonChangeLogin)
    .End();

    BBox* mBoxAroundAddUserName = new BBox("box_addusr",
        B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_FANCY_BORDER, mView);
    mBoxAroundAddUserName->SetLabel(B_TRANSLATE("Change Login"));

    /* UserList */
    mListOfUsers = new BListView("UserList", B_SINGLE_SELECTION_LIST);
    mListOfUsers->SetSelectionMessage(new BMessage(M_APPUSER_SEL));
    mListOfUsers->SetInvocationMessage(new BMessage(M_APPUSER_IVK));
    // BScrollView* userlistScroll = new BScrollView("scv_usrlst", mListOfUsers,
        // B_FOLLOW_LEFT_TOP, 0, false, true, B_FANCY_BORDER);

    BView* mListView = new BView("ListView", B_SUPPORTS_LAYOUT, NULL);
        // new BView(BRect(0, 0, 100, 100), "ListView", 0, 0);
    mListView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    mListView->SetHighUIColor(B_PANEL_TEXT_COLOR);

    mButtonUserMod = new BButton("bt_usrmod", B_TRANSLATE("Modify"),
        new BMessage(M_APPUSER_MODIFY));
    mButtonUserMod->SetEnabled(false);
    mButtonUserRem = new BButton("bt_usrrem", B_TRANSLATE("Remove"),
        new BMessage(M_APPUSER_REMOVE));
    mButtonUserRem->SetEnabled(false);

    BLayoutBuilder::Group<>(mListView, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        // .Add(userlistScroll)
        .Add(mListOfUsers)
        .AddGroup(B_HORIZONTAL)
            .Add(mButtonUserMod)
            .Add(mButtonUserRem)
        .End()
    .End();

    BBox* mBoxAroundListUsers = new BBox("box_lstusr",
        B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_FANCY_BORDER, mListView);
    mBoxAroundListUsers->SetLabel(B_TRANSLATE("Current Users"));

    BView* thisview = new BView(NULL, B_SUPPORTS_LAYOUT, NULL);
    BLayoutBuilder::Group<>(thisview, B_VERTICAL)
        .Add(mBoxAroundAddUserName)
        .Add(mBoxAroundListUsers)
    .End();

    return thisview;
}

BView* mWindow::CreateCardView_Background()
{
    int32 min = 0, max = 255;

    /* Colorcontrol */
    mSpinnerColorR = new BSpinner("sp_col_r", NULL, new BMessage(COLOR_CHANGED_R));
    mSpinnerColorR->SetRange(min, max);
    mSpinnerColorG = new BSpinner("sp_col_g", NULL, new BMessage(COLOR_CHANGED_G));
    mSpinnerColorG->SetRange(min, max);
    mSpinnerColorB = new BSpinner("sp_col_b", NULL, new BMessage(COLOR_CHANGED_B));
    mSpinnerColorB->SetRange(min, max);
    mButtonDefaultColors = new BButton("DefaultColors",
        B_TRANSLATE("Default"), new BMessage(DEFAULT_COLORS));

    BView* mColorView = new BView("ColorView", B_SUPPORTS_LAYOUT, NULL);
    mColorView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    mColorView->SetHighUIColor(B_PANEL_TEXT_COLOR);

    BLayoutBuilder::Group<>(mColorView, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        .AddGrid()
            .Add(new BStringView(NULL, B_TRANSLATE("Red")), 0, 0)
            .Add(mSpinnerColorR, 1, 0)
            .Add(new BStringView(NULL, B_TRANSLATE("Green")), 0, 1)
            .Add(mSpinnerColorG, 1, 1)
            .Add(new BStringView(NULL, B_TRANSLATE("Blue")), 0, 2)
            .Add(mSpinnerColorB, 1, 2)
        .End()
        .Add(mButtonDefaultColors)
    .End();

    BBox* mBoxAroundColorControl = new BBox("box_col",
        B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_FANCY_BORDER, mColorView);
    mBoxAroundColorControl->SetLabel(B_TRANSLATE("Colors"));

    /* ImagePath */

    mTextControlmPathToImageFolder = new BTextControl("TextPathToImages",
        B_TRANSLATE("Path to image folder"),
        settings->BackgroundImageFolderPath(),
        new BMessage(TEXT_IMAGEPATH));
    mTextControlmPathToImageFolder->SetModificationMessage(new BMessage(CHECK_BUTTONS));
    mButtonDefaultImagePath = new BButton("mFrameButtonDefaultImagePath",
        B_TRANSLATE("Default"), new BMessage(BUTTON_DEFAULTPATH));
    mButtonBrowseImagePath = new BButton("mFrameButtonBrowseImagePath",
        B_TRANSLATE("Browse"), new BMessage(BROWSE_FOLDER));
    mSliderBgSnooze = new BSlider("sl_snooze",
        B_TRANSLATE("Image transition delay (in seconds)"),
        new BMessage(M_SNOOZE_SLIDER_CHANGED), 5, 60, B_HORIZONTAL, B_TRIANGLE_THUMB,
        B_FRAME_EVENTS | B_WILL_DRAW | B_NAVIGABLE);
    mSliderBgSnooze->SetHashMarks(B_HASH_MARKS_BOTH);
    mSliderBgSnooze->SetHashMarkCount(12);
    mSliderBgSnooze->SetKeyIncrementValue(5);
    mSliderBgSnooze->SetLimitLabels("5", "60");

    mRadioBtUseBgImgNone = new BRadioButton("rb_folder",
        B_TRANSLATE("Do not use background images"), new BMessage(M_BGMODE_RADIO_NONE));
    mRadioBtUseBgImgFolder = new BRadioButton("rb_folder",
        B_TRANSLATE("Load background images from a folder"), new BMessage(M_BGMODE_RADIO_FOLDER));
    mRadioBtUseBgImgList = new BRadioButton("rb_list",
        B_TRANSLATE("Load background images from a list file"), new BMessage(M_BGMODE_RADIO_LIST));
    mRadioBtUseBgImgList->SetEnabled(false);

    BView* mImageView = new BView("ImageView", B_SUPPORTS_LAYOUT, NULL);
    mImageView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    mImageView->SetHighUIColor(B_PANEL_TEXT_COLOR);

    BLayoutBuilder::Group<>(mImageView, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        .Add(mRadioBtUseBgImgNone)
        .Add(mRadioBtUseBgImgFolder)
        .AddGroup(B_VERTICAL)
            .SetInsets(B_USE_SMALL_INSETS)
            .Add(mTextControlmPathToImageFolder)
            .AddGroup(B_HORIZONTAL)
                .AddGlue()
                .Add(mButtonDefaultImagePath)
                .Add(mButtonBrowseImagePath)
            .End()
        .End()
        .Add(mRadioBtUseBgImgList)
        .AddStrut(4.0f)
        .Add(mSliderBgSnooze)
    .End();

    BBox* mBoxAroundImagePath = new BBox("box_img",
        B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_FANCY_BORDER, mImageView);
    mBoxAroundImagePath->SetLabel(B_TRANSLATE("Background image"));

    BView* thisview = new BView(NULL, B_SUPPORTS_LAYOUT, NULL);
    BLayoutBuilder::Group<>(thisview, B_VERTICAL)
        .Add(mBoxAroundColorControl)
        .Add(mBoxAroundImagePath)
    .End();

    return thisview;
}

BView* mWindow::CreateCardView_Clock()
{
    int32 min = 0, max = 255;
    int32 mMinFontValue = 5, mMaxFontValue = 15;
    int32 HashMarksCount = 11;
    const char* mStringSliderMinLimitLabel =	"10";
    const char* mStringSliderMaxLimitLabel =	"30";
    int CurrentSliderValue = 10;

    /* Clock feature enablement */
    mCheckBoxBoolClock = new BCheckBox("Bool clock",
        B_TRANSLATE("Show Clock? (option currently not available)"), new BMessage(BOOL_CLOCK),
        B_WILL_DRAW | B_NAVIGABLE);

    /* Clock font size */
    mSliderFontSize = new BSlider("FontSizeSlider", B_TRANSLATE("Clock Size"),
        new BMessage(SIZE_SLIDER_CHANGED), mMinFontValue, mMaxFontValue,
        B_HORIZONTAL, B_TRIANGLE_THUMB, B_FRAME_EVENTS | B_WILL_DRAW | B_NAVIGABLE);
    mSliderFontSize->SetHashMarks(B_HASH_MARKS_BOTH);
    mSliderFontSize->SetHashMarkCount(HashMarksCount);
    mSliderFontSize->SetLimitLabels(mStringSliderMinLimitLabel, mStringSliderMaxLimitLabel);
    mSliderFontSize->SetKeyIncrementValue(2);
    mSliderFontSize->SetValue(CurrentSliderValue);

    /* Clock color */
    mSpinnerClockColorR = new BSpinner("sp_clk_r", NULL,
        new BMessage(CLOCKCOLOR_CHANGED_R));
    mSpinnerClockColorR->SetRange(min, max);
    mSpinnerClockColorG = new BSpinner("sp_clk_g", NULL,
        new BMessage(CLOCKCOLOR_CHANGED_G));
    mSpinnerClockColorG->SetRange(min, max);
    mSpinnerClockColorB = new BSpinner("sp_clk_b", NULL,
        new BMessage(CLOCKCOLOR_CHANGED_B));
    mSpinnerClockColorB->SetRange(min, max);
    mButtonDefaultClockColors = new BButton("DefaultClockColors",
        B_TRANSLATE("Default"), new BMessage(DEFAULT_CLOCK_COLORS));

    BView* clockColorView = new BView(NULL, B_SUPPORTS_LAYOUT, NULL);
    clockColorView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    clockColorView->SetHighUIColor(B_PANEL_TEXT_COLOR);

    BLayoutBuilder::Group<>(clockColorView, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        .AddGrid()
            .Add(new BStringView(NULL, B_TRANSLATE("Red")), 0, 0)
            .Add(mSpinnerClockColorR, 1, 0)
            .Add(new BStringView(NULL, B_TRANSLATE("Green")), 0, 1)
            .Add(mSpinnerClockColorG, 1, 1)
            .Add(new BStringView(NULL, B_TRANSLATE("Blue")), 0, 2)
            .Add(mSpinnerClockColorB, 1, 2)
        .End()
        .AddGroup(B_HORIZONTAL)
            .AddGlue()
            .Add(mButtonDefaultClockColors)
        .End()
    .End();

    BBox* boxClockColor = new BBox("box_clock_col",
        B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_FANCY_BORDER, clockColorView);
    boxClockColor->SetLabel(B_TRANSLATE("Colors"));

    /* Clock placement */
    mTextControlClockPlaceX = new BTextControl("ClockPlacementX",
        B_TRANSLATE("Placement X"), "",
        new BMessage(CLOCKPLACEMENT_X));
    mTextControlClockPlaceX->SetModificationMessage(new BMessage(CHECK_BUTTONS));
    mTextControlClockPlaceY = new BTextControl("ClockPlacementY",
        B_TRANSLATE("Placement Y"), "",
        new BMessage(CLOCKPLACEMENT_Y));
    mTextControlClockPlaceY->SetModificationMessage(new BMessage(CHECK_BUTTONS));
    mButtonDefaultClockPlace = new BButton("DefaultPlace",
        B_TRANSLATE("Default"), new BMessage(DEFAULT_PLACE));

    BView* clockPlaceView = new BView(NULL, B_SUPPORTS_LAYOUT, NULL);
    clockPlaceView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    clockPlaceView->SetHighUIColor(B_PANEL_TEXT_COLOR);

    BLayoutBuilder::Group<>(clockPlaceView, B_VERTICAL)
        .SetInsets(B_USE_SMALL_INSETS)
        .AddGrid()
            .AddTextControl(mTextControlClockPlaceX, 0, 0)
            .AddTextControl(mTextControlClockPlaceY, 0, 1)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(new BStringView(NULL, B_TRANSLATE("Position must be within the screen bounds")))
            .AddGlue()
            .Add(mButtonDefaultClockPlace)
        .End()
    .End();

    BBox* boxClockPlace = new BBox("box_clock_loc",
        B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_FANCY_BORDER, clockPlaceView);
    boxClockPlace->SetLabel(B_TRANSLATE("Placement"));

    BView* thisview = new BView("ClockView", B_SUPPORTS_LAYOUT, NULL);
    thisview->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    thisview->SetHighUIColor(B_PANEL_TEXT_COLOR);

    BLayoutBuilder::Group<>(thisview, B_VERTICAL)
        .SetInsets(0)
        .Add(mCheckBoxBoolClock)
        .Add(mSliderFontSize)
        .Add(boxClockColor)
        .Add(boxClockPlace)
    .End();

    return thisview;
}

BView* mWindow::CreateCardView_Options()
{
    mCheckBoxSessionBar = new BCheckBox("cb_sb",
        B_TRANSLATE("Show session bar (shutdown, restart)"), new BMessage(M_BOOL_SESSION));
    mCheckBoxSysInfo = new BCheckBox("cb_si",
        B_TRANSLATE("Show system information panel"), new BMessage(M_BOOL_INFO));
    mCheckBoxKillerShortcut = new BCheckBox("cb_ks",
        B_TRANSLATE("Enable shortcut (Cmd+Ctrl+Space) to bypass password protection."),
        new BMessage(M_BOOL_KILLER));
    mCheckBoxEventLog = new BCheckBox("cb_el",
        B_TRANSLATE("Enable login events logging"), new BMessage());

    BView* thisview = new BView(NULL, B_SUPPORTS_LAYOUT, NULL);
    BLayoutBuilder::Group<>(thisview, B_VERTICAL)
        .Add(mCheckBoxSessionBar)
        .Add(mCheckBoxSysInfo)
        .Add(mCheckBoxKillerShortcut)
        .Add(mCheckBoxEventLog)
        .AddGlue()
    .End();

    return thisview;
}

// #pragma mark - Checks

bool mWindow::UI_IsDefault(BMessage* defaults)
{
    /* background */
    bool isBgModeDefault = mRadioBtUseBgImgFolder->Value() == defaults->GetUInt8(mNameConfigBgMode, 1);
    bool isBgColorDefault = UI_IsBgColorDefault(defaults);
    bool isImgFolderPathDefault = UI_IsBgFolderDefault(defaults);
    bool isBgSnoozeDefault = mSliderBgSnooze->Value() == defaults->GetUInt32(mNameConfigBgSnooze, 10);

    /* clock */
    bool isClockColorDefault = UI_IsClockColorDefault(defaults);
    bool isClockPlacementDefault = UI_IsClockPlaceDefault(defaults);
    bool isClockHidden = mCheckBoxBoolClock->Value() == defaults->GetBool(mNameConfigBoolClock);
    bool isClockSizeDefault = mSliderFontSize->Value() == defaults->GetUInt32(mNameConfigClockFontSize, 8);

    /* options */
    bool isSessionBarDefault = mCheckBoxSessionBar->Value() == defaults->GetBool(mNameConfigSessionBarOn);
    bool isSysInfoPanelDefault = mCheckBoxSysInfo->Value() == defaults->GetBool(mNameConfigSysInfoPanelOn);
    bool isKillerShctDefault = mCheckBoxKillerShortcut->Value() == defaults->GetBool(mNameConfigKillerShortcutOn);
    bool isLoggingDefault = mCheckBoxEventLog->Value() == defaults->GetBool(mNameConfigEvtLoggingOn);

    return isBgModeDefault && isBgColorDefault && isImgFolderPathDefault &&
           isBgSnoozeDefault && isClockColorDefault && isClockPlacementDefault &&
           isClockHidden && isClockSizeDefault && isSessionBarDefault &&
           isSysInfoPanelDefault && isKillerShctDefault && isLoggingDefault;
}

bool mWindow::UI_IsBgColorDefault(BMessage* archive)
{
    return
        (uint8)mSpinnerColorR->Value() == archive->GetColor(mNameConfigBgColor, {}).red &&
        (uint8)mSpinnerColorG->Value() == archive->GetColor(mNameConfigBgColor, {}).green &&
        (uint8)mSpinnerColorB->Value() == archive->GetColor(mNameConfigBgColor, {}).blue;
}

bool mWindow::UI_IsBgFolderDefault(BMessage* archive)
{
    return strcmp(mTextControlmPathToImageFolder->Text(),
        archive->GetString(mNameConfigImagePath)) == 0;
}

bool mWindow::UI_IsClockColorDefault(BMessage* archive)
{
    return
        (uint8)mSpinnerClockColorR->Value() == archive->GetColor(mNameConfigClockColor, {}).red &&
        (uint8)mSpinnerClockColorG->Value() == archive->GetColor(mNameConfigClockColor, {}).green &&
        (uint8)mSpinnerClockColorB->Value() == archive->GetColor(mNameConfigClockColor, {}).blue;
}

bool mWindow::UI_IsClockPlaceDefault(BMessage* archive)
{
    return
        atoi(mTextControlClockPlaceX->Text()) == archive->GetPoint(mNameConfigClockPlace, {}).x &&
        atoi(mTextControlClockPlaceY->Text()) == archive->GetPoint(mNameConfigClockPlace, {}).y;
}

