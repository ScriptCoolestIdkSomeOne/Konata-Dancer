//just copied this file from my other project
/*#include "XPMessageBox.h"

void RunHappy99(HINSTANCE hInstance);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    auto res1 = XPMessageBox::Show(NULL, L"test1", L"TEST1",
        XPMessageBox::IconType::Question,
        XPMessageBox::ButtonType::YesNo,
        XPMessageBox::DialogResult::Yes,
        XPMessageBox::WindowMode::Dialog,
        0, 0, 1.0f, 0.9f);

    auto res2 = XPMessageBox::Show(NULL, L"test2", L"TEST2",
        XPMessageBox::IconType::Warning,
        XPMessageBox::ButtonType::OKCancel,
        XPMessageBox::DialogResult::OK,
        XPMessageBox::WindowMode::Dialog,
        0, 0, 1.5f, 1.5f,
        nullptr,  //callback
        {  //button actions shit
            {XPMessageBox::DialogResult::OK, [hInstance]() {
            RunHappy99(hInstance);//DOESNT WORK, don't do that
        }}
        });

    //builder pattern
    auto res3 = XPMessageBox::MessageBoxBuilder()
        .SetParent(NULL)
        .SetMessage(L"happy99 shit")
        .SetTitle(L"happy99")
        .SetIcon(XPMessageBox::IconType::Question)
        .SetButtons(XPMessageBox::ButtonType::YesNo)
        .SetDefaultButton(XPMessageBox::DialogResult::Yes)
        .OnButton(XPMessageBox::DialogResult::Yes, [hInstance]() {
        RunHappy99(hInstance);
            })
        .OnButton(XPMessageBox::DialogResult::No, []() {
        MessageBoxW(NULL, L"test complete", L"nah", MB_OK);
            })
        .Show();

    auto res4 = XPMessageBox::MessageBoxBuilder()
        .SetParent(NULL)
        .SetMessage(L"exe test")
        .SetTitle(L"exe test")
        .SetButtons(XPMessageBox::ButtonType::YesNo)
        .OnButton(XPMessageBox::DialogResult::Yes, []() {
        XPMessageBox::RunExecutable(L"notepad.exe");
            })
        .Show();

    auto res6 = XPMessageBox::Show(NULL, L"test3", L"TEST3",
        XPMessageBox::IconType::Information,
        XPMessageBox::ButtonType::OK,
        XPMessageBox::DialogResult::OK,
        XPMessageBox::WindowMode::Dialog,
        1000, 500, 1.0f, 2.0f);

    if (res6 == XPMessageBox::DialogResult::OK) {//doesnt work, don't do that too
        RunHappy99(hInstance);
    }

    auto res7 = XPMessageBox::Show(NULL, L"test4", L"TEST4",
        XPMessageBox::IconType::Error,
        XPMessageBox::ButtonType::OK,
        XPMessageBox::DialogResult::OK,
        XPMessageBox::WindowMode::Dialog,
        0, 0, 0.7f, 0.7f);

    auto res75 = XPMessageBox::MessageBoxBuilder()
        .SetParent(NULL)
        .SetMessage(L"mp3")
        .SetTitle(L"MP3")
        .SetMP3Sound(L"Congratulations_You've_Won.mp3")
        .SetButtons(XPMessageBox::ButtonType::OK)
        .Show();

    auto res8 = XPMessageBox::MessageBoxBuilder()
        .SetParent(NULL)
        .SetMessage(L"coolest")
        .SetTitle(L"sounking")
        .SetIcon(XPMessageBox::IconType::Question)
        .SetButtons(XPMessageBox::ButtonType::YesNo)
        .SetSound(L"Congratulations_You've_Won.wav")
        .Show();

    XPMessageBox::Show(NULL, L"soundsing", L"sound",
        XPMessageBox::IconType::Information);
    PlaySoundW(L"Congratulations_You've_Won.wav", NULL, SND_FILENAME | SND_ASYNC);//im too lazy to make a .wav file but i know that this shit works
    return 0;
}
//now kids you know how to use this piece of SSS-s-s-s-software*/