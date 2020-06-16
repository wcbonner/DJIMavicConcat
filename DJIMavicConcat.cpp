// DJIMavicConcat.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "DJIMavicConcat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only application object

CWinApp theApp;

using namespace std;

/////////////////////////////////////////////////////////////////////////////
CString FindEXEFromPath(const CString & csEXE)
{
	CString csFullPath;
	//PathFindOnPath();
	CFileFind finder;
	if (finder.FindFile(csEXE))
	{
		finder.FindNextFile();
		csFullPath = finder.GetFilePath();
		finder.Close();
	}
	else
	{
		TCHAR filename[MAX_PATH];
		unsigned long buffersize = sizeof(filename) / sizeof(TCHAR);
		// Get the file name that we are running from.
		GetModuleFileName(AfxGetResourceHandle(), filename, buffersize);
		PathRemoveFileSpec(filename);
		PathAppend(filename, csEXE);
		if (finder.FindFile(filename))
		{
			finder.FindNextFile();
			csFullPath = finder.GetFilePath();
			finder.Close();
		}
		else
		{
			CString csPATH;
			csPATH.GetEnvironmentVariable(_T("PATH"));
			int iStart = 0;
			CString csToken(csPATH.Tokenize(_T(";"), iStart));
			while (csToken != _T(""))
			{
				if (csToken.Right(1) != _T("\\"))
					csToken.AppendChar(_T('\\'));
				csToken.Append(csEXE);
				if (finder.FindFile(csToken))
				{
					finder.FindNextFile();
					csFullPath = finder.GetFilePath();
					finder.Close();
					break;
				}
				csToken = csPATH.Tokenize(_T(";"), iStart);
			}
		}
	}
	return(csFullPath);
}
/////////////////////////////////////////////////////////////////////////////
static const CString QuoteFileName(const CString & Original)
{
	CString csQuotedString(Original);
	if (csQuotedString.Find(_T(" ")) >= 0)
	{
		csQuotedString.Insert(0, _T('"'));
		csQuotedString.AppendChar(_T('"'));
	}
	return(csQuotedString);
}
/////////////////////////////////////////////////////////////////////////////
std::string timeToISO8601(const time_t & TheTime)
{
	std::ostringstream ISOTime;
	//time_t timer = TheTime;
	struct tm UTC;// = gmtime(&timer);
	if (0 == gmtime_s(&UTC, &TheTime))
		//if (UTC != NULL)
	{
		ISOTime.fill('0');
		if (!((UTC.tm_year == 70) && (UTC.tm_mon == 0) && (UTC.tm_mday == 1)))
		{
			ISOTime << UTC.tm_year + 1900 << "-";
			ISOTime.width(2);
			ISOTime << UTC.tm_mon + 1 << "-";
			ISOTime.width(2);
			ISOTime << UTC.tm_mday << "T";
		}
		ISOTime.width(2);
		ISOTime << UTC.tm_hour << ":";
		ISOTime.width(2);
		ISOTime << UTC.tm_min << ":";
		ISOTime.width(2);
		ISOTime << UTC.tm_sec;
	}
	return(ISOTime.str());
}
std::wstring getTimeISO8601(void)
{
	time_t timer;
	time(&timer);
	std::string isostring(timeToISO8601(timer));
	std::wstring rval;
	rval.assign(isostring.begin(), isostring.end());

	return(rval);
}
time_t ISO8601totime(const std::string & ISOTime)
{
	struct tm UTC;
	UTC.tm_year = atol(ISOTime.substr(0, 4).c_str()) - 1900;
	UTC.tm_mon = atol(ISOTime.substr(5, 2).c_str()) - 1;
	UTC.tm_mday = atol(ISOTime.substr(8, 2).c_str());
	UTC.tm_hour = atol(ISOTime.substr(11, 2).c_str());
	UTC.tm_min = atol(ISOTime.substr(14, 2).c_str());
	UTC.tm_sec = atol(ISOTime.substr(17, 2).c_str());
#ifdef _MSC_VER
	_tzset();
	_get_daylight(&(UTC.tm_isdst));
#endif
	time_t timer = mktime(&UTC);
#ifdef _MSC_VER
	long Timezone_seconds = 0;
	_get_timezone(&Timezone_seconds);
	timer -= Timezone_seconds;
	int DST_hours = 0;
	_get_daylight(&DST_hours);
	long DST_seconds = 0;
	_get_dstbias(&DST_seconds);
	timer += DST_hours * DST_seconds;
#endif
	return(timer);
}
std::string timeToExcelDate(const time_t & TheTime)
{
	std::ostringstream ISOTime;
	time_t timer = TheTime;
	// TODO: Get rid of this deprecated call to gmtime()
	#pragma warning(suppress : 4996)
	struct tm * UTC = gmtime(&timer);
	if (UTC != NULL)
	{
		ISOTime.fill('0');
		ISOTime << UTC->tm_year + 1900 << "-";
		ISOTime.width(2);
		ISOTime << UTC->tm_mon + 1 << "-";
		ISOTime.width(2);
		ISOTime << UTC->tm_mday << " ";
		ISOTime.width(2);
		ISOTime << UTC->tm_hour << ":";
		ISOTime.width(2);
		ISOTime << UTC->tm_min << ":";
		ISOTime.width(2);
		ISOTime << UTC->tm_sec;
	}
	return(ISOTime.str());
}
/////////////////////////////////////////////////////////////////////////////
void csvline_populate(std::vector<std::string> &record, const std::string& line, char delimiter)
{
	int linepos = 0;
	int inquotes = false;
	int linemax = line.length();
	std::string curstring;
	record.clear();

	while (line[linepos] != 0 && linepos < linemax)
	{
		char c = line[linepos];
		if (!inquotes && curstring.length() == 0 && c == '"')
		{
			//beginquotechar
			inquotes = true;
		}
		else if (inquotes && c == '"')
		{
			//quotechar
			if ((linepos + 1 < linemax) && (line[linepos + 1] == '"'))
			{
				//encountered 2 double quotes in a row (resolves to 1 double quote)
				curstring.push_back(c);
				linepos++;
			}
			else
			{
				//endquotechar
				inquotes = false;
			}
		}
		else if (!inquotes && c == delimiter)
		{
			//end of field
			record.push_back(curstring);
			curstring = "";
		}
		else if (!inquotes && (c == '\r' || c == '\n'))
		{
			record.push_back(curstring);
			return;
		}
		else
		{
			curstring.push_back(c);
		}
		linepos++;
	}
	record.push_back(curstring);
	return;
}
/////////////////////////////////////////////////////////////////////////////
bool SplitPath(
	CString csSrcPath,
	CString & DestDir,
	CString & DestFileName,
	CString & DestFileExt
)
{
	bool rval = false;
	TCHAR FQN[_MAX_PATH];
	GetFullPathName(csSrcPath.GetString(), _MAX_PATH, FQN, NULL);
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	errno_t err = _tsplitpath_s(FQN, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
	if (err == 0)
	{
		rval = true;
		DestDir = CString(drive) + CString(dir);
		DestFileName = CString(fname);
		DestFileExt = CString(ext);
	}
	return(rval);
}
/////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "version")
CString GetFileVersion(const CString & filename)
{
	CString rval;
	// get The Version number of the file
	DWORD dwVerHnd = 0;
	DWORD nVersionInfoSize = ::GetFileVersionInfoSize((LPTSTR)filename.GetString(), &dwVerHnd);
	if (nVersionInfoSize > 0)
	{
		UINT *puVersionLen = new UINT;
		LPVOID pVersionInfo = new char[nVersionInfoSize];
		BOOL bTest = ::GetFileVersionInfo((LPTSTR)filename.GetString(), dwVerHnd, nVersionInfoSize, pVersionInfo);
		// Pull out the version number
		if (bTest)
		{
			LPVOID pVersionNum = NULL;
			bTest = ::VerQueryValue(pVersionInfo, _T("\\"), &pVersionNum, puVersionLen);
			if (bTest)
			{
				DWORD dwFileVersionMS = ((VS_FIXEDFILEINFO *)pVersionNum)->dwFileVersionMS;
				DWORD dwFileVersionLS = ((VS_FIXEDFILEINFO *)pVersionNum)->dwFileVersionLS;
				rval.Format(_T("%d.%d.%d.%d"), HIWORD(dwFileVersionMS), LOWORD(dwFileVersionMS), HIWORD(dwFileVersionLS), LOWORD(dwFileVersionLS));
			}
		}
		delete puVersionLen;
		delete[] pVersionInfo;
	}
	return(rval);
}
/////////////////////////////////////////////////////////////////////////////
CString GetFileVersionString(const CString & filename, const CString & string)
{
	CString rval;
	// get The Version number of the file
	DWORD dwVerHnd = 0;
	DWORD nVersionInfoSize = ::GetFileVersionInfoSize((LPTSTR)filename.GetString(), &dwVerHnd);
	if (nVersionInfoSize > 0)
	{
		UINT *puVersionLen = new UINT;
		LPVOID pVersionInfo = new char[nVersionInfoSize];
		BOOL bTest = ::GetFileVersionInfo((LPTSTR)filename.GetString(), dwVerHnd, nVersionInfoSize, pVersionInfo);
		if (bTest)
		{
			// Structure used to store enumerated languages and code pages.
			struct LANGANDCODEPAGE {
				WORD wLanguage;
				WORD wCodePage;
			}	*lpTranslate = NULL;
			unsigned int cbTranslate;
			// Read the list of languages and code pages.
			::VerQueryValue(pVersionInfo, _T("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate);
			// Read the file description for each language and code page.
			LPVOID lpSubBlockValue = NULL;
			unsigned int SubBlockValueSize = 1;
			for (unsigned int i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++)
			{
				CString SubBlockName;
				SubBlockName.Format(_T("\\StringFileInfo\\%04x%04x\\%s"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage, string.GetString());
				// Retrieve file description for language and code page "i". 
				::VerQueryValue(pVersionInfo, (LPTSTR)SubBlockName.GetString(), &lpSubBlockValue, &SubBlockValueSize);
			}
			if (SubBlockValueSize > 0)
				rval = CString((LPTSTR)lpSubBlockValue, SubBlockValueSize - 1);
		}
		delete puVersionLen;
		delete[] pVersionInfo;
	}
	return(rval);
}
/////////////////////////////////////////////////////////////////////////////
const CString GetLogFileName()
{
	static CString csLogFileName;
	if (csLogFileName.IsEmpty())
	{
		TCHAR szLogFilePath[MAX_PATH] = _T("");
		SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szLogFilePath);
		std::wostringstream woss;
		woss << AfxGetAppName();
		time_t timer;
		time(&timer);
		struct tm UTC;
		if (!gmtime_s(&UTC, &timer))
		{
			woss << "-";
			woss.fill('0');
			woss << UTC.tm_year + 1900 << "-";
			woss.width(2);
			woss << UTC.tm_mon + 1;
		}
		PathAppend(szLogFilePath, woss.str().c_str());
		PathAddExtension(szLogFilePath, _T(".txt"));
		csLogFileName = CString(szLogFilePath);
	}
	return(csLogFileName);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
			std::wcout << "Fatal Error: MFC initialization failed" << std::endl;
            nRetCode = 1;
        }
        else
        {
			if (argc < 4)
			{
				std::wcout << "command Line Format:" << std::endl;
				std::wcout << "\t" << argv[0] << " VideoName PathToFirstFile.mp4 [PathToNextFile.mp4] PathToLastFile.mp4" << std::endl;
			}
			else
			{
				std::vector<CString> SourceVideoList;
				std::vector<CString> SourceSubtitleList;
				CString csFFMPEGPath(FindEXEFromPath(_T("ffmpeg.exe")));
				CString csVideoName(argv[1]);
				{
					TCHAR drive[_MAX_DRIVE];
					TCHAR dir[_MAX_DIR];
					TCHAR fname[_MAX_FNAME];
					TCHAR ext[_MAX_EXT];
					errno_t err = _tsplitpath_s(csVideoName.GetString(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
					if (err == 0)
					{
						TCHAR fullpath[_MAX_PATH];
						_tmakepath_s(fullpath, drive, dir, fname, _T("mkv"));
						csVideoName = CString(fullpath);
					}
				}
				std::locale mylocale("");	// get global locale
				std::wcout.imbue(mylocale);	// imbue global locale
				for (auto argindex = 2; argindex < argc;)
				{
					CFileStatus StatusVideo;
					if (TRUE == CFile::GetStatus(argv[argindex++], StatusVideo))
					{
						SourceVideoList.push_back(StatusVideo.m_szFullName);
						TCHAR drive[_MAX_DRIVE];
						TCHAR dir[_MAX_DIR];
						TCHAR fname[_MAX_FNAME];
						TCHAR ext[_MAX_EXT];
						errno_t err = _tsplitpath_s(StatusVideo.m_szFullName, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
						if (err == 0)
						{
							TCHAR fullpath[_MAX_PATH];
							_tmakepath_s(fullpath, drive, dir, fname, _T("srt"));
							CFileStatus StatusSubTitle;
							if (TRUE == CFile::GetStatus(fullpath, StatusSubTitle))
								SourceSubtitleList.push_back(StatusSubTitle.m_szFullName);
						}
					}
				}
				std::wofstream m_LogFile(GetLogFileName().GetString(), std::ios_base::out | std::ios_base::app | std::ios_base::ate);
				if (m_LogFile.is_open())
				{
					TCHAR tcHostName[256] = TEXT("");
					DWORD dwSize = sizeof(tcHostName);
					GetComputerNameEx(ComputerNameDnsHostname, tcHostName, &dwSize);
					std::locale mylocale("");   // get global locale
					m_LogFile.imbue(mylocale);  // imbue global locale
					m_LogFile << "[" << getTimeISO8601() << "] LogFile Opened (" << tcHostName << ")" << std::endl;
					TCHAR filename[1024];
					unsigned long buffersize = sizeof(filename) / sizeof(TCHAR);
					// Get the file name that we are running from.
					GetModuleFileName(AfxGetResourceHandle(), filename, buffersize);
					m_LogFile << "[                   ] Program: " << CStringA(filename).GetString() << std::endl;
					m_LogFile << "[                   ] Version: " << CStringA(GetFileVersion(filename)).GetString() << " Built: " __DATE__ " at " __TIME__ << std::endl;
					m_LogFile << "[                   ] Command: ";
					for (auto index = 0; index < argc; index++) m_LogFile << QuoteFileName(argv[index]).GetString() << " "; m_LogFile << std::endl;
					m_LogFile << "[" << getTimeISO8601() << "] " << "Total Files: " << SourceVideoList.size() << std::endl;
					m_LogFile << "[" << getTimeISO8601() << "] " << "  File List:";
					for (auto SourceFile = SourceVideoList.begin(); SourceFile != SourceVideoList.end(); SourceFile++)
						m_LogFile << " " << SourceFile->GetString();
					m_LogFile << std::endl;
					m_LogFile.close();
				}

				if (csFFMPEGPath.GetLength() > 0)
				{
					vector<wchar_t *> args;
					vector<wstring> mycommand;
					mycommand.push_back(csFFMPEGPath.GetString());
#ifdef _DEBUG
					mycommand.push_back(_T("-report"));
#else
					mycommand.push_back(_T("-hide_banner"));
#endif
					CString csFilterCommand;
					// create temp file with one input file on each line
					// Gets the temp path env string (no guarantee it's a valid path).
					TCHAR lpTempPathBuffer[MAX_PATH];
					auto dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
					if (dwRetVal > MAX_PATH || (dwRetVal == 0))
						std::wcout << "[" << getTimeISO8601() << "] " << "GetTempPath failed" << std::endl;
					else
					{
						TCHAR szTempFileNameMP4[MAX_PATH] = _T("");
						TCHAR szTempFileNameSRT[MAX_PATH] = _T("");
						//  Generates a temporary file name. 
						if ((0 == GetTempFileName(lpTempPathBuffer, _T("Wim"), 0, szTempFileNameMP4)) || (0 == GetTempFileName(lpTempPathBuffer, _T("Wim"), 0, szTempFileNameSRT)))
							std::wcout << "[" << getTimeISO8601() << "] " << "GetTempFileName failed" << std::endl;
						else
						{
							std::wofstream TmpFileMP4(szTempFileNameMP4, std::ios_base::out | std::ios_base::app | std::ios_base::ate);
							std::wofstream TmpFileSRT(szTempFileNameSRT, std::ios_base::out | std::ios_base::app | std::ios_base::ate);
							if (TmpFileMP4.is_open() && TmpFileSRT.is_open())
							{
								for (auto SourceFile = SourceVideoList.begin(); SourceFile != SourceVideoList.end(); SourceFile++)
								{
									CString TmpString(QuoteFileName(SourceFile->GetString()).GetString());
									TmpString.Replace(_T("\\"), _T("/"));
									TmpFileMP4 << _T("file ") << TmpString.GetString() << std::endl;
								}
								TmpFileMP4.close();
								mycommand.push_back(_T("-f")); mycommand.push_back(_T("concat"));
								mycommand.push_back(_T("-safe")); mycommand.push_back(_T("0"));
								mycommand.push_back(_T("-i")); mycommand.push_back(szTempFileNameMP4);
								for (auto SourceFile = SourceSubtitleList.begin(); SourceFile != SourceSubtitleList.end(); SourceFile++)
								{
									CString TmpString(QuoteFileName(SourceFile->GetString()).GetString());
									TmpString.Replace(_T("\\"), _T("/"));
									TmpFileSRT << _T("file ") << TmpString.GetString() << std::endl;
								}
								TmpFileSRT.close();
								mycommand.push_back(_T("-f")); mycommand.push_back(_T("concat"));
								mycommand.push_back(_T("-safe")); mycommand.push_back(_T("0"));
								mycommand.push_back(_T("-i")); mycommand.push_back(szTempFileNameSRT);
								mycommand.push_back(_T("-map")); mycommand.push_back(_T("0:v"));
								mycommand.push_back(_T("-map")); mycommand.push_back(_T("1"));
								mycommand.push_back(_T("-c:v")); mycommand.push_back(_T("copy"));
								mycommand.push_back(_T("-c:s")); mycommand.push_back(_T("copy"));
#ifndef _DEBUG
								mycommand.push_back(_T("-n"));
#else
								mycommand.push_back(_T("-y"));
#endif
								mycommand.push_back(QuoteFileName(csVideoName).GetString());
								std::wcout << "[" << getTimeISO8601() << "]";
								m_LogFile.open(GetLogFileName().GetString(), std::ios_base::out | std::ios_base::app | std::ios_base::ate);
								if (m_LogFile.is_open())
									m_LogFile << "[" << getTimeISO8601() << "]";
								for (auto arg = mycommand.begin(); arg != mycommand.end(); arg++)
								{
									std::wcout << " " << *arg;
									if (m_LogFile.is_open())
										m_LogFile << " " << *arg;
									args.push_back((wchar_t *)arg->c_str());
								}
								std::wcout << std::endl;
								if (m_LogFile.is_open())
								{
									m_LogFile << std::endl;
									m_LogFile.close();
								}
								args.push_back(NULL);

								CTime ctStart(CTime::GetCurrentTime());
								CTimeSpan ctsTotal = CTime::GetCurrentTime() - ctStart;

								if (-1 == _tspawnvp(_P_WAIT, csFFMPEGPath.GetString(), &args[0])) //HACK: This really needs to be figured out, but I think it should work.
									std::wcout << "[" << getTimeISO8601() << "]  _tspawnvp failed: " /* << _sys_errlist[errno] */ << std::endl;

								ctsTotal = CTime::GetCurrentTime() - ctStart;
								auto TotalSeconds = ctsTotal.GetTotalSeconds();
								if (TotalSeconds > 0)
								{
									auto FPS = double(SourceVideoList.size()) / double(TotalSeconds);
									std::wcout << "[" << getTimeISO8601() << "] encoded " << SourceVideoList.size() << " files in " << TotalSeconds << "s (" << FPS << "fps)" << std::endl;
									m_LogFile.open(GetLogFileName().GetString(), std::ios_base::out | std::ios_base::app | std::ios_base::ate);
									if (m_LogFile.is_open())
									{
										m_LogFile << "[" << getTimeISO8601() << "] encoded " << SourceVideoList.size() << " files in " << TotalSeconds << "s (" << FPS << "fps)" << std::endl;
										m_LogFile.close();
									}
								}

								// If we created temporary files, delete them.
								CFileStatus StatusTmpFile;
								if (TRUE == CFile::GetStatus(szTempFileNameMP4, StatusTmpFile))
								{
									std::wcout << "[" << getTimeISO8601() << "] Removing Temporary File: " << StatusTmpFile.m_szFullName << std::endl;
									DeleteFile(StatusTmpFile.m_szFullName);
								}
								if (TRUE == CFile::GetStatus(szTempFileNameSRT, StatusTmpFile))
								{
									std::wcout << "[" << getTimeISO8601() << "] Removing Temporary File: " << StatusTmpFile.m_szFullName << std::endl;
									DeleteFile(StatusTmpFile.m_szFullName);
								}

								// Adjust the date on the newly created file to have the same date as the first of the source files and report the final file size
								try
								{
									CFileStatus StatusFirstVideo;
									CFileStatus StatusFinalVideo;
									if (TRUE == CFile::GetStatus(SourceVideoList.begin()->GetString(), StatusFirstVideo))
										if (TRUE == CFile::GetStatus(csVideoName, StatusFinalVideo))
										{
											m_LogFile.open(GetLogFileName().GetString(), std::ios_base::out | std::ios_base::app | std::ios_base::ate);
											if (m_LogFile.is_open())
											{
												m_LogFile << "[" << getTimeISO8601() << "] Final File Size: " << (StatusFinalVideo.m_size >> 20) << " MB" << std::endl;
												m_LogFile.close();
											}
											StatusFinalVideo.m_ctime = StatusFinalVideo.m_mtime = StatusFirstVideo.m_ctime;
											CFile::SetStatus(csVideoName, StatusFinalVideo);
										}
								}
								catch (CFileException *e)
								{
									TCHAR   szCause[512];
									e->GetErrorMessage(szCause, sizeof(szCause) / sizeof(TCHAR));
									CStringA csErrorMessage(szCause);
									csErrorMessage.Trim();
									std::wstringstream ss;
									ss << "[" << getTimeISO8601() << "] CFileException: (" << e->m_lOsError << ") " << csErrorMessage.GetString() << std::endl;
									TRACE(ss.str().c_str());
									m_LogFile.open(GetLogFileName().GetString(), std::ios_base::out | std::ios_base::app | std::ios_base::ate);
									if (m_LogFile.is_open())
									{
										m_LogFile << ss.str();
										m_LogFile.close();
									}
								}

								// run FFMPEG a second time to output the subtitles into a single concatenated file for youtube upload, since youtube doesn't seem to recognize embedded subtitles
								mycommand.clear();
								args.clear();
								mycommand.push_back(csFFMPEGPath.GetString());
#ifdef _DEBUG
								mycommand.push_back(_T("-report"));
								mycommand.push_back(_T("-y")); // overwrite output files while debugging without prompting
#else
								mycommand.push_back(_T("-hide_banner"));
								mycommand.push_back(_T("-n"));
#endif
								mycommand.push_back(_T("-i")); mycommand.push_back(QuoteFileName(csVideoName).GetString());
								{
									TCHAR drive[_MAX_DRIVE];
									TCHAR dir[_MAX_DIR];
									TCHAR fname[_MAX_FNAME];
									TCHAR ext[_MAX_EXT];
									errno_t err = _tsplitpath_s(csVideoName.GetString(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
									if (err == 0)
									{
										TCHAR fullpath[_MAX_PATH];
										_tmakepath_s(fullpath, drive, dir, fname, _T("srt"));
										mycommand.push_back(fullpath);
									}
								}
								std::wcout << "[" << getTimeISO8601() << "]";
								m_LogFile.open(GetLogFileName().GetString(), std::ios_base::out | std::ios_base::app | std::ios_base::ate);
								if (m_LogFile.is_open())
									m_LogFile << "[" << getTimeISO8601() << "]";
								for (auto arg = mycommand.begin(); arg != mycommand.end(); arg++)
								{
									std::wcout << " " << *arg;
									if (m_LogFile.is_open())
										m_LogFile << " " << *arg;
									args.push_back((wchar_t *)arg->c_str());
								}
								std::wcout << std::endl;
								if (m_LogFile.is_open())
								{
									m_LogFile << std::endl;
									m_LogFile.close();
								}
								args.push_back(NULL);
								if (-1 == _tspawnvp(_P_WAIT, csFFMPEGPath.GetString(), &args[0])) //HACK: This really needs to be figured out, but I think it should work.
									std::wcout << "[" << getTimeISO8601() << "]  _tspawnvp failed: " /* << _sys_errlist[errno] */ << std::endl;
							}
						}
					}
				}
				m_LogFile.open(GetLogFileName().GetString(), std::ios_base::out | std::ios_base::app | std::ios_base::ate);
				if (m_LogFile.is_open())
				{
					m_LogFile << "[" << getTimeISO8601() << "] LogFile Closed" << std::endl;
					m_LogFile.close();
				}
			}
        }
    }
    else
    {
        // TODO: change error code to suit your needs
		std::wcout << "Fatal Error: GetModuleHandle failed" << std::endl;
        nRetCode = 1;
    }

    return nRetCode;
}
