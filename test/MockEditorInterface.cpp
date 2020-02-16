// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "MockEditorInterface.h"
#include "CommonFunctions.h"
#include "utils/enum_range.h"
#include <filesystem>
#include "utils/string_utils.h"

void MockedDocumentInfo::set_data(const std::wstring &data_arg) {
  switch (codepage) {
  case EditorCodepage::ansi:
    set_data_raw (to_string(data_arg));
    break;
  case EditorCodepage::utf8:
    set_data_raw (to_utf8_string(data_arg));
    break;
  case EditorCodepage::COUNT:
    break;
  }
}

void MockedDocumentInfo::set_data_raw(const std::string &data_arg) {
  cur.data = data_arg;
  cur.style.resize(cur.data.size());
  cur.selection = {0, 0};
  cursor_pos = 0;
  for (auto &info : indicator_info)
    std::fill(info.set_for.begin(), info.set_for.end(), false);
}

void MockedDocumentInfo::erase(TextPosition start, TextPosition length) {
  cur.data.erase(cur.data.begin() + start, cur.data.begin() + start + length);
  cur.style.erase(cur.style.begin() + start,
                  cur.style.begin() + start + length);
}

void MockedDocumentInfo::save_state() { past.push_back(cur); }

void MockEditorInterface::move_active_document_to_other_view() {
  auto &view = m_documents[m_active_view];
  auto doc = std::move(view[m_active_document_index[m_active_view]]);
  view.erase(view.begin() + m_active_document_index[m_active_view]);
  auto other = other_view(m_active_view);
  m_documents[other].push_back(std::move(doc));
  m_active_document_index[other] =
      static_cast<int>(m_documents[other].size()) - 1;
}

void MockEditorInterface::add_toolbar_icon(
    int /*cmd_id*/, const toolbarIcons * /*tool_bar_icons_ptr*/) {}

void MockEditorInterface::force_style_update(
  TextPosition /*from*/, TextPosition /*to*/) {}

void MockEditorInterface::set_selection(TextPosition from,
                                        TextPosition to) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  doc->cursor_pos = to;
  if (from < to)
    doc->cur.selection = {from, to};
  else
    doc->cur.selection = {to, from};
}

void MockEditorInterface::replace_selection(
  const char *str) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  auto &d = doc->cur.data;
  d.replace(doc->cur.selection[0], doc->cur.selection[1], str);
  doc->cur.style.resize (doc->cur.data.length ());
}

void MockEditorInterface::set_indicator_style(
  int indicator_index, int style) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  if (indicator_index >= static_cast<int>(doc->indicator_info.size()))
    doc->indicator_info.resize(indicator_index + 1);
  doc->indicator_info[indicator_index].style = style;
}

void MockEditorInterface::set_indicator_foreground(
  int indicator_index,
  int style) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  if (indicator_index >= static_cast<int>(doc->indicator_info.size()))
    doc->indicator_info.resize(indicator_index + 1);
  doc->indicator_info[indicator_index].foreground = style;
}

void MockEditorInterface::set_current_indicator(
  int indicator_index) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  if (indicator_index >= static_cast<int>(doc->indicator_info.size()))
    doc->indicator_info.resize(indicator_index + 1);
  doc->current_indicator = indicator_index;
}

void MockEditorInterface::indicator_fill_range(TextPosition from,
                                               TextPosition to) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  auto &s = doc->indicator_info[doc->current_indicator].set_for;
  if (to >= static_cast<int>(s.size()))
    s.resize(to + 1);
  std::fill(s.begin() + from, s.begin() + to, true);
}

void MockEditorInterface::indicator_clear_range(TextPosition from,
                                                TextPosition to) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  auto &s = doc->indicator_info[doc->current_indicator].set_for;
  if (to >= static_cast<TextPosition>(s.size()))
    s.resize(to + 1);
  std::fill(s.begin() + from, s.begin() + to, false);
}

NppViewType MockEditorInterface::active_view() const {
  return m_active_view;
}

EditorCodepage MockEditorInterface::get_encoding(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return EditorCodepage::utf8;
  return doc->codepage;
}

TextPosition MockEditorInterface::get_current_pos(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->cursor_pos;
}

int MockEditorInterface::get_current_line_number(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<int>(
      std::count(doc->cur.data.begin(),
                 doc->cur.data.begin() + get_current_pos(view), '\n'));
}

int MockEditorInterface::get_text_height(NppViewType /*view*/,
                                         int /*line*/) const {
  return text_height;
}

int MockEditorInterface::line_from_position(NppViewType view,
                                            TextPosition position) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<int>(std::count(doc->cur.data.begin(),
                                     doc->cur.data.begin() + position, '\n'));
}

TextPosition MockEditorInterface::get_line_start_position(NppViewType view,
                                                          TextPosition line) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  size_t index = 0;
  for (int i = 0; i < line; ++i) {
    index = doc->cur.data.find('\n', index);
    if (index == std::wstring::npos)
      return static_cast<TextPosition>(doc->cur.data.size());

    ++index;
  }
  return static_cast<TextPosition>(index);
}

TextPosition MockEditorInterface::get_line_end_position(NppViewType view,
                                                        TextPosition line) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  size_t index = 0;
  for (int i = 0; i < line + 1; ++i) {
    index = doc->cur.data.find('\n', index);
    if (index == std::wstring::npos)
      return static_cast<TextPosition>(doc->cur.data.size());
    if (i != line + 1)
      ++index;
  }
  return static_cast<TextPosition>(index);
}

int MockEditorInterface::get_lexer(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return 0;
  return doc->lexer;
}

TextPosition MockEditorInterface::get_selection_start(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->cur.selection[0];
}

TextPosition MockEditorInterface::get_selection_end(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->cur.selection[1];
}

int MockEditorInterface::get_style_at(NppViewType view,
                                      TextPosition position) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->cur.style[position];
}

bool MockEditorInterface::is_style_hotspot(NppViewType view,
                                           int style) const {
  auto doc = active_document(view);
  if (!doc)
    return false;
  return doc->hotspot_style == style;
}

TextPosition MockEditorInterface::get_active_document_length(
    NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<TextPosition>(doc->cur.data.length());
}

TextPosition MockEditorInterface::get_line_length(NppViewType view, int line) const {
  size_t index = 0;
  auto doc = active_document(view);
  if (!doc)
    return -1;
  for (int i = 0; i < line; ++i) {
    index = doc->cur.data.find('\n', index);
    if (index == std::wstring::npos)
      return -1;
    ++index;
  }
  return static_cast<TextPosition>(doc->cur.data.find('\n', index) - index);
}

int MockEditorInterface::get_point_x_from_position(NppViewType view,
                                                   TextPosition position) const {
  return static_cast<int>(position -
          get_line_start_position(view, line_from_position(view, position))) *
         10;
}

int MockEditorInterface::get_point_y_from_position(NppViewType view,
                                                   TextPosition position) const {
  auto line = line_from_position(view, position);
  return line * get_text_height(view, line);
}

TextPosition MockEditorInterface::get_first_visible_line(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->visible_lines[0];
}

TextPosition MockEditorInterface::get_lines_on_screen(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  auto &l = doc->visible_lines;
  return l[1] - l[0] + 1;
}

TextPosition MockEditorInterface::get_document_line_from_visible(
    NppViewType view, TextPosition visible_line) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->visible_lines[0] + visible_line;
}

TextPosition MockEditorInterface::get_document_line_count(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<TextPosition>(
      std::count(doc->cur.data.begin(), doc->cur.data.end(), '\n'));
}

bool MockEditorInterface::open_document(std::wstring filename) {
  assert(false);
  return false;
}

void MockEditorInterface::activate_document(int index) {
  m_active_view = m_target_view;
  m_active_document_index[m_target_view] = index;
}

void MockEditorInterface::activate_document(const std::wstring &filepath) {
  auto it =
      std::find_if(m_documents[m_target_view].begin(), m_documents[m_target_view].end(),
                   [&](const auto &data) { return data.path == filepath; });
  if (it != m_documents[m_target_view].end()) {
    m_active_view = m_target_view;
    m_active_document_index[m_target_view] =
        static_cast<int>(it - m_documents[m_target_view].begin());
  }
}

void MockEditorInterface::switch_to_file(const std::wstring &path) {
  for (auto view : enum_range<NppViewType>())
    {
      set_target_view(static_cast<int> (view));
      activate_document(path);
    }
}

bool MockEditorInterface::is_opened(const std::wstring &filename) const {
  for (auto view : enum_range<NppViewType>())
    if (std::find_if(m_documents[view].begin(), m_documents[view].end(),
                     [&](const auto &data) { return data.path == filename; }) !=
        m_documents[view].end())
      return true;
  return false;
}

std::wstring MockEditorInterface::active_document_path() const {
  auto doc = active_document(m_active_view);
  if (!doc)
    return L"";
  return doc->path;
}

std::wstring MockEditorInterface::active_file_directory() const {
  auto doc = active_document(m_active_view);
  if (!doc)
    return L"";
  return std::filesystem::path(doc->path).parent_path();
}

std::wstring MockEditorInterface::plugin_config_dir() const { return L""; }

std::string MockEditorInterface::selected_text(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc || doc->cur.selection[0] < 0 || doc->cur.selection[1] < 0)
    return "";
  return doc->cur.data.substr(doc->cur.selection[0],
                              doc->cur.selection[1] - doc->cur.selection[0]);
}

std::string MockEditorInterface::get_current_line(NppViewType view) const {
  return get_line(view, get_current_line_number(view));
}

std::string MockEditorInterface::get_line(NppViewType view,
                                          TextPosition line_number) const {
  auto start = get_line_start_position(view, line_number);
  auto end = get_line_end_position(view, line_number);
  return get_text_range(view, start, end);
}

std::optional<TextPosition> MockEditorInterface::char_position_from_global_point(
    NppViewType /*view*/, int /*x*/, int /*y*/) const {
  return std::nullopt;
}

HWND MockEditorInterface::get_editor_hwnd() const { return nullptr; }

std::wstring MockEditorInterface::get_full_current_path() const {
  auto doc = active_document(m_active_view);
  if (!doc)
    return L"";
  return doc->path;
}

std::string MockEditorInterface::get_text_range(NppViewType view, TextPosition from,
                                                TextPosition to) const {
  auto doc = active_document(view);
  if (!doc)
    return "";
  return doc->cur.data.substr(from, to - from);
}

std::string
MockEditorInterface::get_active_document_text(NppViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return "";
  return doc->cur.data;
}

TextPosition MockEditorInterface::char_position_from_point(NppViewType view,
                                                   const POINT &pnt) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return get_line_start_position(
      view, get_document_line_from_visible(view, pnt.y / text_height)) +
         pnt.x / text_width;
}

RECT MockEditorInterface::editor_rect(NppViewType /*view*/) const {
  RECT r = {0, 0, 10000, 10000};
  return r;
}

MockEditorInterface::MockEditorInterface() {
  m_active_document_index.fill(-1);
  m_save_undo.fill(true);
}

MockEditorInterface::~MockEditorInterface() = default;

void MockEditorInterface::open_virtual_document(NppViewType view,
                                                const std::wstring &path,
                                                const std::wstring &data) {
  MockedDocumentInfo info;
  info.path = path;
  info.set_data(data);
  info.codepage = EditorCodepage::utf8;
  m_documents[view].push_back(std::move(info));
  m_active_view = view;
  m_active_document_index[view] =
      static_cast<int>(m_documents[view].size() - 1);
}

void MockEditorInterface::set_active_document_text(NppViewType view,
                                                   const std::wstring &text) {
  auto doc = active_document(view);
  if (doc)
    doc->set_data(text);
}

void MockEditorInterface::set_active_document_text_raw(NppViewType view,
  const std::string &text) {
  auto doc = active_document(view);
  if (doc)
    doc->set_data_raw(text);
}

std::vector<std::string>
MockEditorInterface::get_underlined_words(NppViewType view,
                                          int indicator_id) const {
  auto doc = active_document(view);
  if (!doc)
    return {};
  if (indicator_id >= static_cast<int>(doc->indicator_info.size()))
    return {};
  auto &target = doc->indicator_info[indicator_id].set_for;
  auto it = target.begin(), jt = target.begin();
  std::vector<std::string> result;
  while (true) {
    it = std::find(jt, target.end(), true);
    if (it == target.end())
      return result;
    jt = std::find(it, target.end(), false);
    result.push_back(get_text_range(view,
                                    static_cast<TextPosition>(it - target.begin()),
                                    static_cast<TextPosition>(jt - target.begin())));
  }
}

void MockEditorInterface::make_all_visible(NppViewType view) {
  auto doc = active_document(view);
  if (!doc)
    return;

  doc->visible_lines = {0, get_document_line_count(view)};
}

void MockEditorInterface::set_lexer(NppViewType view, int lexer) {
  auto doc = active_document(view);
  if (!doc)
    return;
  doc->lexer = lexer;
}

void MockEditorInterface::set_whole_text_style(NppViewType view, int style) {
  auto doc = active_document(view);
  if (!doc)
    return;
  std::fill(doc->cur.style.begin(), doc->cur.style.end(), style);
}

void MockEditorInterface::set_codepage(NppViewType view,
                                       EditorCodepage codepage) {
  auto doc = active_document(view);
  if (!doc)
    return;
  doc->codepage = codepage;
}

void MockEditorInterface::delete_range(TextPosition start,
                                       TextPosition length) {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  if (m_save_undo[m_target_view])
    doc->save_state();
  doc->erase(start, length);
}

void MockEditorInterface::begin_undo_action(NppViewType view) {
  for (auto &doc : m_documents[view])
    doc.save_state();
  m_save_undo[view] = false;
}

void MockEditorInterface::end_undo_action(NppViewType view) {
  m_save_undo[view] = true;
}

void MockEditorInterface::undo() {
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  // For now redo is not supported
  if (!doc->past.empty()) {
    doc->cur = doc->past.back();
    doc->past.pop_back();
  }
}

bool MockEditorInterface::is_line_visible(NppViewType /*view*/, TextPosition /*line*/) const
{
  return true;
}

TextPosition MockEditorInterface::find_next(TextPosition from_position, const char* needle)
{
  auto doc = active_document(m_target_view);
  if (!doc)
    return -1;

  auto pos = find_case_insensitive (std::string_view (doc->cur.data).substr(from_position), needle);
  if (pos == std::string::npos)
    return -1;

  return static_cast<TextPosition> (pos) + from_position;
}

void MockEditorInterface::replace_text(TextPosition from, TextPosition to, std::string_view replacement)
{
  auto doc = active_document(m_target_view);
  if (!doc)
    return;
  auto &d = doc->cur.data;
  d.replace(from, to - from, replacement);
  doc->cur.style.resize (doc->cur.data.length ());
}

void MockEditorInterface::add_bookmark(TextPosition /*line*/)
{

  assert(!"Unsupported by mock editor");
}

constexpr auto mock_editor_view_count = 2;

int MockEditorInterface::get_view_count() const
{
  return mock_editor_view_count;
}

void MockEditorInterface::set_target_view(int view_index)
{
  m_target_view = static_cast<NppViewType> (view_index);
}

int MockEditorInterface::get_target_view() const
{
  return static_cast<int> (m_target_view);
}

std::vector<std::wstring> MockEditorInterface::get_open_filenames() const
{
  std::vector<std::wstring> out;
  for (auto view : enum_range<NppViewType>())
    if (view == m_target_view)
      std::transform(m_documents[view].begin(), m_documents[view].end(),
                     std::back_inserter(out),
                     [](const auto &data) { return data.path; });
  return out;
}

std::vector<std::wstring> MockEditorInterface::get_open_filenames_all_views() const
{
  std::vector<std::wstring> out;
  for (auto view : enum_range<NppViewType>())
      std::transform(m_documents[view].begin(), m_documents[view].end(),
                     std::back_inserter(out),
                     [](const auto &data) { return data.path; });
  return out;
}

MockedDocumentInfo *MockEditorInterface::active_document(NppViewType view) {
  if (m_documents[view].empty())
    return nullptr;
  return &m_documents[view][m_active_document_index[view]];
}

const MockedDocumentInfo *
MockEditorInterface::active_document(NppViewType view) const {
  if (m_documents[view].empty())
    return nullptr;
  return &m_documents[view][m_active_document_index[view]];
}
