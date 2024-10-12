#include "window_class.hpp"

window_create::window_create(std::wstring const& title) noexcept
:m_title(title) 
{
    window_settings();
    
    m_main_window_handle = CreateWindowEx(
        0,                              // Optional window styles.
        m_c_name.c_str(),                     // Window class
        m_title.c_str(),    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        LoadMenu(m_hinst, MAKEINTRESOURCE(IDR_MENU1)), // Load the menu here
        m_hinst,  // Instance handle
        this        // Additional application data
    );

    ShowWindow(m_main_window_handle, SW_SHOW);
}

LRESULT window_create::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    // reroute to private window proc

    window_create* p_window_rerouter = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        // Store the pointer to the window instance in the user data associated with the HWND.
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        p_window_rerouter = (window_create*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p_window_rerouter);
    }
    else
    {
        // Retrieve the pointer to the window instance from the user data associated with the HWND.
        p_window_rerouter = (window_create*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (p_window_rerouter)
    {
        return p_window_rerouter->PrivateWindowProc(hwnd, uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT window_create::PrivateWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (uMsg){
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // All painting occurs here, between BeginPaint and EndPaint.

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);

        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_NEWWINDOW_CREATE: // menu button called "Create New Window"
        {
            std::thread* new_window_thread_p = new std::thread(&window_create::create_relative, this);

            // this thread will now wait until "new_window_thread_p" has finished executing the function
            // "create_relative()"
            {
                std::mutex mtx_window_created;
                std::unique_lock<std::mutex> lock(mtx_window_created);
                m_cv_window_created.wait(lock, [this] { return m_window_created_gate_state.load(); });  // Wait until the gate is open
                
                // now close the gate
                m_window_created_gate_state = false;
            }


            auto new_window_thread_id = new_window_thread_p->get_id();
            auto found_relative_window_pointer = m_relative_mp.find(new_window_thread_id);
            m_thread_mp.emplace(new_window_thread_p, found_relative_window_pointer->second);
            window_relative* new_relative_window_p = found_relative_window_pointer->second;
            new_relative_window_p->set_thread_p(new_window_thread_p);


            // notify "new_window_thread_p" that it is safe to proceed in the "create_relative()" function
            m_window_proc_case_ID_NEWWINDOW_CREATE = true; // open seseame
            m_cv_window_proc_case_ID_NEWWINDOW_CREATE.notify_all(); // say it dont spray it

            break;
        }
        
        } // end of switch (LOWORD(wParam))
    } // end of switch (uMsg)

    // no default switches needed
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void window_create::window_settings() noexcept{

    m_wc.lpfnWndProc = WindowProc;
    m_wc.hInstance = m_hinst;
    m_wc.lpszClassName = m_c_name.c_str();

    RegisterClass(&m_wc);
}

void window_create::create_relative() noexcept
{
    // increase open window count by 1
    ++m_open_window_count;
    
    window_relative* new_window = new window_relative(m_c_name, m_title, m_hinst,m_main_window_handle);
    m_relative_mp.emplace(new_window->get_id(), new_window);

    // open the gate
    m_window_created_gate_state = true;

    // notify that the window was created
    m_cv_window_created.notify_all();

    // wait here for the current "case ID_NEWWINDOW_CREATE:" in "window_create::PrivateWindowProc(...)" function to finish then it is safe to proceed
    {
        std::mutex mtx_window_proc_case_ID_NEWWINDOW_CREATE;
        std::unique_lock<std::mutex> lock(mtx_window_proc_case_ID_NEWWINDOW_CREATE);
        m_cv_window_proc_case_ID_NEWWINDOW_CREATE.wait(lock, [this] { return m_window_proc_case_ID_NEWWINDOW_CREATE.load(); });  // Wait until the gate is open
        
        // shut the gate behind me
        m_window_proc_case_ID_NEWWINDOW_CREATE = false;
    }

    // allow the thread to process win32 messages
    this->run_relative_message_pump(new_window->get_id());
}

void window_create::close_relative() noexcept
{
    auto this_thread_id = std::this_thread::get_id();
    auto found_relative_window_p = m_relative_mp.find(this_thread_id);
    if (found_relative_window_p != m_relative_mp.end()) {
        window_relative* current_relative_window_p = found_relative_window_p->second;
        
        // make sure to detach the this thread
        current_relative_window_p->get_thread_p()->detach();

        delete current_relative_window_p;
        m_relative_mp.erase(found_relative_window_p);
    }

    // decrease window count by 1
    --m_open_window_count;

    // if there are no more relative windows open notify the main thread
    if (m_relative_mp.empty() == true) {
        // notify the main message loop in the winmain function to check if all windows are closed
        // and if they are it is safe to exit the program
        m_public_open_window_count_gate_state = true;
        m_public_cv_open_window_count.notify_all();
    }
}

void window_create::run_relative_message_pump(std::thread::id this_thread_id) noexcept
{
    // get the relative window pointer
    auto found_relative_window_pointer = m_relative_mp.find(this_thread_id);
    window_relative* relative_window_pointer = found_relative_window_pointer->second;
    
    // launch the any logic code here
    std::thread logic_thread(&window_create::run_logic_changes, this,relative_window_pointer);
    
    // Run the message loop.
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // cause the while loop in "run_logic_changes(...)" to exit
    relative_window_pointer->m_public_p_running_logic->store(false);
    
    // make sure to join the "logic_thread" before closing the relative window
    logic_thread.join();
    this->close_relative();
}

void window_create::run_logic_changes(window_relative* active_open_window) noexcept
{
    std::wstring funky_title = L"Evil Window";
    while (active_open_window->m_public_p_running_logic->load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        funky_title = funky_title + L"*";

        if (funky_title.size() == 50) {
            // reset 
            funky_title = L"Evil Window";
        }

        active_open_window->change_title(funky_title);
    }
}

window_relative::window_relative(std::wstring const& c_name, std::wstring const& title,HINSTANCE hinst,HWND main_handle) noexcept
{
    m_window_handle = CreateWindowEx(
        0,                              // Optional window styles.
        c_name.c_str(),                     // Window class
        title.c_str(),    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        LoadMenu(hinst, MAKEINTRESOURCE(IDR_MENU1)), // Load the menu here
        hinst,  // Instance handle
        reinterpret_cast<window_create*>(GetWindowLongPtr(main_handle, GWLP_USERDATA)) // Additional application data
    );

    m_relative_id = std::this_thread::get_id();

    ShowWindow(m_window_handle, SW_SHOW);
}

void window_relative::change_title(std::wstring const& new_title) noexcept
{
    SetWindowTextW(this->m_window_handle, new_title.c_str());
}
