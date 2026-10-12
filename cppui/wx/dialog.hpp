#ifndef CPPUI_WX_DIALOG
#define CPPUI_WX_DIALOG
#include "../optional.hpp"
#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/filename.h>
#include <sstream>
#include <vector>
namespace CPPUI { namespace Wx { namespace detail {
inline wxString fileFilters(std::string types) {
    if(types.empty())return wxFileSelectorDefaultWildcardStr;
    for(std::size_t p=0;(p=types.find(";;",p))!=std::string::npos;)types.replace(p,2,"|");
    std::vector<std::string> parts;std::stringstream stream(types);std::string part;
    while(std::getline(stream,part,'|'))parts.push_back(part);
    bool qt=!parts.empty();for(auto & p:parts)qt=qt && p.find('(')!=std::string::npos && p.back()==')';
    if(!qt)return wxString::FromUTF8(types);
    std::string result;for(auto & p:parts){if(!result.empty())result+='|';result+=p+'|';std::istringstream patterns(p.substr(p.rfind('(')+1,p.size()-p.rfind('(')-2));std::string pattern;bool first=true;while(patterns>>pattern){if(!first)result+=';';result+=pattern;first=false;}}
    return wxString::FromUTF8(result);
}
}}}
namespace CPPUI {
inline OptionalText OpenDirectory(const std::string & title="Open Directory",const std::string & directory="") {
    wxDirDialog dialog(nullptr,wxString::FromUTF8(title),wxString::FromUTF8(directory),wxDD_DEFAULT_STYLE|wxDD_DIR_MUST_EXIST);
    return dialog.ShowModal()==wxID_OK?OptionalText(dialog.GetPath().ToStdString(wxConvUTF8)):OptionalText();
}
inline OptionalText OpenFile(const std::string & title="Open File",const std::string & directory="",const std::string & types="") {
    wxFileDialog dialog(nullptr,wxString::FromUTF8(title),wxString::FromUTF8(directory),"",Wx::detail::fileFilters(types),wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    return dialog.ShowModal()==wxID_OK?OptionalText(dialog.GetPath().ToStdString(wxConvUTF8)):OptionalText();
}
inline Optional<std::vector<std::string>> OpenFiles(const std::string & title="Open Files",const std::string & directory="",const std::string & types="") {
    wxFileDialog dialog(nullptr,wxString::FromUTF8(title),wxString::FromUTF8(directory),"",Wx::detail::fileFilters(types),wxFD_OPEN|wxFD_FILE_MUST_EXIST|wxFD_MULTIPLE);
    if(dialog.ShowModal()!=wxID_OK)return {};
    wxArrayString paths;dialog.GetPaths(paths);std::vector<std::string> result;for(auto & path:paths)result.push_back(path.ToStdString(wxConvUTF8));return result;
}
inline OptionalText SaveFile(OptionalText defaultValue={},const std::string & title="Save File",const std::string & directory="",const std::string & types="") {
    wxFileName path(wxString::FromUTF8(defaultValue.valueOr("")));
    wxFileDialog dialog(nullptr,wxString::FromUTF8(title),directory.empty()?path.GetPath():wxString::FromUTF8(directory),path.GetFullName(),Wx::detail::fileFilters(types),wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    return dialog.ShowModal()==wxID_OK?OptionalText(dialog.GetPath().ToStdString(wxConvUTF8)):OptionalText();
}
inline OptionalText SaveFile(const std::string & value,const std::string & title="Save File",const std::string & directory="",const std::string & types=""){return SaveFile(OptionalText(value),title,directory,types);}
inline void Information(const std::string & message="Information",const std::string & title="Information Dialog"){wxMessageBox(wxString::FromUTF8(message),wxString::FromUTF8(title),wxOK|wxICON_INFORMATION);}
inline void Warning(const std::string & message="Warning",const std::string & title="Warning Dialog"){wxMessageBox(wxString::FromUTF8(message),wxString::FromUTF8(title),wxOK|wxICON_WARNING);}
inline void Critical(const std::string & message="Critical",const std::string & title="Critical Dialog"){wxMessageBox(wxString::FromUTF8(message),wxString::FromUTF8(title),wxOK|wxICON_ERROR);}
inline bool Confirm(const std::string & message="Confirm",const std::string & title="Confirm Dialog"){wxMessageDialog dialog(nullptr,wxString::FromUTF8(message),wxString::FromUTF8(title),wxYES_NO|wxICON_QUESTION);return dialog.ShowModal()==wxID_YES;}
inline OptionalText Prompt(const std::string & prompt="Input",const std::string & title="Input Dialog",const std::string & value=""){
    wxTextEntryDialog dialog(nullptr,wxString::FromUTF8(prompt),wxString::FromUTF8(title),wxString::FromUTF8(value));return dialog.ShowModal()==wxID_OK?OptionalText(dialog.GetValue().ToStdString(wxConvUTF8)):OptionalText();
}
}
#endif
