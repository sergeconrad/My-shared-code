#pragma once
#include <Windows.h>

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Diagnostics;
using namespace System::Threading;

namespace Term {

	/// <summary>
	/// Summary for ShellConsoleControl
	/// </summary>
	public ref class ShellConsoleControl : public System::Windows::Forms::UserControl
	{	
	public:
		enum class ShellType : int { Command, Powershell };

	public:
		ShellConsoleControl(void) 
			: shellType_(ShellType::Command)
		{
			InitializeComponent();
		}
		ShellConsoleControl(ShellType type, Form^ parentForm)
			: shellType_(type)
			, parentForm_(parentForm)
		{
			InitializeComponent();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~ShellConsoleControl()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->SuspendLayout();
			// 
			// ShellConsoleControl
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::SystemColors::Desktop;
			this->Name = L"ShellConsoleControl";
			this->Size = System::Drawing::Size(734, 486);
			this->Load += gcnew System::EventHandler(this, &ShellConsoleControl::ShellConsoleControl_Load);
			this->Resize += gcnew System::EventHandler(this, &ShellConsoleControl::ShellConsoleControl_Resize);
			this->ResumeLayout(false);

		}
#pragma endregion
	
	private:
		// CONSTS
		//LPCWSTR SHELL_CMD_EXE_PATH = L"C:/WINDOWS/System32/cmd.exe";
		LPCWSTR SHELL_CMD_EXE_PATH = L"cmd.exe";
		//LPCWSTR SHELL_PS_EXE_PATH = L"C:\\Windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe";
		LPCWSTR SHELL_PS_EXE_PATH = L"powershell.exe";
		const int WATCH_TIMER_INTERVAL = 1000;

		// members
		ShellType shellType_;
		Process^ shell_ = nullptr;
		String^ shellTitle_ = String::Empty;
		System::Windows::Forms::Timer^ watchTimer_;
		Form^ parentForm_ = nullptr;

	public:
		event EventHandler^ Exited;

	public:
		void SetFocus() {
			Debug::WriteLine(__FUNCTION__);
			if (this->IsShellWindowCreated()) {
				::SetForegroundWindow((HWND)shell_->MainWindowHandle.ToPointer());
			}
			return;
		}
		void KillShellProcess() {
			// ignore exception on shutdown
			try {
				Debug::WriteLine(__FUNCTION__);
				RemoveTimer();
				if (shell_ != nullptr) {
					Debug::Write(shell_->Id.ToString() + " to be killed... ");
					if (shell_->HasExited) {
						Debug::WriteLine(" already exited.");						
					}
					else {
						shell_->EnableRaisingEvents = false; // not raise Exited on shutdown
						shell_->Kill();
						Debug::WriteLine("Killed!");
					}
				}
			}
			catch (Exception^ ex) {
				Debug::WriteLine(__FUNCTION__ + " Exception: " + ex->Message);
			}
			return;
		}
		String^ GetShellWindowTitle() { 
			//Debug::WriteLine(__FUNCTION__);
			String^ title = String::Empty;
			if (this->IsShellWindowCreated()) {
				TCHAR titleChars[1024];
				GetWindowText((HWND)shell_->MainWindowHandle.ToPointer(), titleChars, 1024);
				title = gcnew String(titleChars);
			}
			return title;//shell_->MainWindowTitle; 
		}
		void SetShellWindowTitle(LPCWSTR title) {
			Debug::WriteLine(__FUNCTION__);
			SendMessage((HWND)shell_->MainWindowHandle.ToPointer(), WM_SETTEXT, 0, (LPARAM)title);
			return;
		}
		
	private: 
		System::Void ShellConsoleControl_Load(System::Object^  sender, System::EventArgs^  e) {
			Debug::WriteLine(__FUNCTION__);
			CreateShellWindow();
			return;
		}
		System::Void ShellConsoleControl_Resize(System::Object^  sender, System::EventArgs^  e) {
			Debug::WriteLine(__FUNCTION__);
			ResizeShellWindow();
			return;
		}
		
		void OnShellExited(System::Object ^sender, System::EventArgs ^e) {
			Debug::WriteLine(__FUNCTION__);
			// cleanup
			RemoveTimer();
			if (shell_->HasExited) {
				Debug::WriteLine(__FUNCTION__+ " " + shell_->Id + " exited. Disposing...");
				delete shell_;
				shell_ = nullptr;
			}
			// raise exited
			Exited(this, nullptr);
			return;
		}
		void OnWatchTimerTick(System::Object ^sender, System::EventArgs ^e) {
			String^ title = GetShellWindowTitle();
			if (!String::IsNullOrEmpty(title)) {
				if (title != shellTitle_) {
					dynamic_cast<TabPage^>(this->Parent)->Text = title;
					shellTitle_ = title;
				}
			}
			return;
		}

	private:
		// CREATE SHELL PROCESS AND HIJACK IT
		void CreateShellWindow() {
			Debug::WriteLine(__FUNCTION__);
			
			// create shell console
			shell_ = gcnew Process();

			String^ shellExec = String::Empty;
			switch (shellType_) {
				case ShellType::Command: {
					shellExec = gcnew String(SHELL_CMD_EXE_PATH);
					break;
				}
				case ShellType::Powershell: {
					shellExec = gcnew String(SHELL_PS_EXE_PATH);
					break;
				}
				default: {
					shellExec = gcnew String(SHELL_CMD_EXE_PATH);
					break;
				}
			}

			shell_->StartInfo->FileName = shellExec; // gcnew String(SHELL_CMD_EXE_PATH);
			shell_->StartInfo->WindowStyle = ProcessWindowStyle::Minimized;

			// watch cmd command 'exit' (command prompt> exit)
			shell_->EnableRaisingEvents = true;
			shell_->Exited += gcnew System::EventHandler(this, &Term::ShellConsoleControl::OnShellExited);
			
			// start shell process
			shell_->Start();

			// it takes a bit to start
			SpinWait::SpinUntil(gcnew Func<bool>(this, &ShellConsoleControl::IsShellWindowCreated));
			HWND shellHwnd = (HWND)shell_->MainWindowHandle.ToPointer();

			// embed shell console window in the control
			SetParent(shellHwnd, (HWND)(this->Handle.ToPointer()));
			//::SetWindowPos(	shellHwnd, NULL, 4, 0, this->ClientSize.Width - 8, this->ClientSize.Height, SWP_NOZORDER | SWP_NOACTIVATE);
			::SetWindowPos(	shellHwnd, NULL, 0, 0, 
				this->ClientSize.Width, this->ClientSize.Height, SWP_NOZORDER | SWP_NOACTIVATE);

			// remove title bar and borders of shell console window
			LONG style = GetWindowLong(shellHwnd, GWL_STYLE);			
			SetWindowLong(shellHwnd, GWL_STYLE, style & ~WS_SYSMENU);
			SetWindowLong(shellHwnd, GWL_STYLE, style & ~WS_CAPTION);
			SetWindowLong(shellHwnd, GWL_STYLE, style & ~WS_THICKFRAME); // disable resize
			SetWindowLong(shellHwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);

			shellTitle_ = GetShellWindowTitle();
			//AddWatchTitleTimer();
			// 
			Debug::WriteLine(__FUNCTION__ + " Tab: '" + shell_->Id + " " + shellTitle_ + "' created.'");
			return;
		}
		void ResizeShellWindow() {
			Debug::WriteLine(__FUNCTION__);
			if (this->IsShellWindowCreated()) {
				if (parentForm_ != nullptr && parentForm_->WindowState == FormWindowState::Minimized) {
					Debug::WriteLine(__FUNCTION__ + " - MainForm is minimaized");
				}
				else {
					::SetWindowPos(
						(HWND)shell_->MainWindowHandle.ToPointer(), NULL, 0, 0,
						this->ClientSize.Width - 8, this->ClientSize.Height,
						SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				}
			}
			return;
		}
		void AddWatchTitleTimer() {
			watchTimer_ = gcnew System::Windows::Forms::Timer();
			watchTimer_->Interval = WATCH_TIMER_INTERVAL;
			watchTimer_->Tick += gcnew System::EventHandler(this, &Term::ShellConsoleControl::OnWatchTimerTick);
			watchTimer_->Start();
			return;
		}
		void RemoveTimer() {
			Debug::WriteLine(__FUNCTION__);
			try {
				if (watchTimer_ != nullptr) {
					watchTimer_->Stop();
					delete watchTimer_;
					watchTimer_ = nullptr;
				}
			}
			catch (Exception^ ex) {
				Debug::WriteLine(__FUNCTION__ + " Excepton: " + ex->Message);
			}
			return;
		}

		inline bool IsShellWindowCreated() {
			return (shell_ != nullptr) && (shell_->MainWindowHandle != IntPtr::Zero);
		}	
		inline LONG GetWindowLong(HWND windowHandle, int index) {
			if (IntPtr::Size == 4) { // 32-bit arch.
				return ::GetWindowLong(windowHandle, index);
			}
			return ::GetWindowLongPtr(windowHandle, index);
		}
		inline LONG SetWindowLong(HWND windowHandle, int index, LONG newValue) {
			if (IntPtr::Size == 4) { // 64-bit arch. 
				return ::SetWindowLong(windowHandle, index, newValue);
			}
			return ::SetWindowLongPtr(windowHandle, index, newValue);
		}

	protected: 	
		// not used now
		virtual void WndProc(Message% m) override {
			UserControl::WndProc(m);
			return;
		}

	private:

};
}




