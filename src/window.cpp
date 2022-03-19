#include <gtksourceviewmm/init.h>
#include <gtksourceviewmm/languagemanager.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include "window.hpp"
#include "util.hpp"

Novelest::Novelest(Glib::RefPtr<Gtk::Application> app)
{
    m_app = app;
    set_title("Novelest");
    set_default_size(800, 600);

    if (!init_builder()) {
        m_app->quit();
    } else {
        init_store();
        init_workspace();
        init_handlers();
        init_menu();
    }
}

bool Novelest::init_builder()
{
    m_builder = Gtk::Builder::create();
    try {
        m_builder->add_from_file("/home/jechasteen/Repos/novelest/window.ui");
    } catch (const Glib::FileError& err) {
        std::cerr << "FileError: " << err.what() << std::endl;
        return false;
    } catch (const Glib::MarkupError& err) {
        std::cerr << "MarkupError: " << err.what() << std::endl;
        return false;
    } catch (const Gtk::BuilderError& err) {
        std::cerr << "BuilderError: " << err.what() << std::endl;
        return false;
    }

    return true;
}

void Novelest::init_store()
{
    m_store = Gtk::ListStore::create(m_columns);

    m_builder->get_widget("organizer", m_treeview);
    m_treeview->set_model(m_store);
    m_treeview->set_reorderable(true);

    m_treeview->append_column_editable("Title", m_columns.m_col_title);
    m_treeview->get_column(0)->set_resizable(true);
    auto title_column = dynamic_cast<Gtk::CellRendererText*>(m_treeview->get_column_cell_renderer(0));
    title_column->signal_edited().connect(sigc::mem_fun(*this, &Novelest::on_chapter_title_edited));

    m_treeview->append_column_editable("?", m_columns.m_col_include);
    auto include_column = dynamic_cast<Gtk::CellRendererToggle*>(m_treeview->get_column_cell_renderer(1));
    include_column->signal_toggled().connect(sigc::mem_fun(*this, &Novelest::on_include_toggled));

    m_selection = m_treeview->get_selection();
    m_selection->signal_changed().connect(sigc::mem_fun(*this, &Novelest::on_selection_changed));
}

void Novelest::init_workspace()
{
    m_builder->get_widget("grid", m_grid);

    if (m_grid)
        add(*m_grid);

    Gsv::init();

    Glib::RefPtr<Gsv::LanguageManager> manager = Gsv::LanguageManager::get_default();
    m_builder->get_widget("editor", m_editor);
    m_editor->get_source_buffer()->set_language(manager->get_language("markdown"));
    m_editor->set_sensitive(false);

    m_builder->get_widget("progress_word_count", m_progress_word_count);
    m_progress_word_count->set_text("");
}

void Novelest::init_handlers()
{
    Gtk::Button* m_btn_new_chapter;
    m_builder->get_widget("btn_new_chapter", m_btn_new_chapter);
    m_btn_new_chapter->set_sensitive(false);
    m_btn_new_chapter->signal_clicked().connect(sigc::mem_fun(*this, &Novelest::on_new_chapter));

    Gtk::Button* btn_del_chapter;
    m_builder->get_widget("btn_del_chapter", btn_del_chapter);
    btn_del_chapter->set_sensitive(false);
}

void Novelest::init_menu()
{
    Gtk::MenuButton* btn_file_menu;
    m_builder->get_widget("btn_file_menu", btn_file_menu);
    btn_file_menu->set_label("File");

    // File
    Gtk::MenuItem* menu_quit;
    m_builder->get_widget("menu_quit", menu_quit);
    menu_quit->signal_activate().connect(sigc::mem_fun(*this, &Novelest::on_menu_quit));

    Gtk::MenuItem* menu_save_as;
    m_builder->get_widget("menu_save_as", menu_save_as);
    menu_save_as->signal_activate().connect(sigc::mem_fun(*this, &Novelest::on_menu_save_as));

    Gtk::MenuItem* menu_open;
    m_builder->get_widget("menu_open", menu_open);
    menu_open->signal_activate().connect(sigc::mem_fun(*this, &Novelest::on_menu_open));

    Gtk::MenuItem* menu_new;
    m_builder->get_widget("menu_new", menu_new);
    menu_new->signal_activate().connect(sigc::mem_fun(*this, &Novelest::on_menu_new));

    // Import Export
    Gtk::MenuItem* menu_import;
    m_builder->get_widget("menu_import", menu_import);
    menu_import->signal_activate().connect(sigc::mem_fun(*this, &Novelest::on_menu_import));
}

void Novelest::update_word_count()
{
    auto chapters = m_db->get_chapters();
    double total = 0;
    for (auto c : chapters) {
        if (c.include)
            total += word_count(c.body);
    }
    double target = m_db->get_target();
    double percent = round((total / target) * 100);
    std::stringstream text;
    text << total << " / " << target << " Words (" << percent << "%)";
    m_progress_word_count->set_fraction(total / target);
    m_progress_word_count->set_text(text.str());
}

void Novelest::import(std::vector<std::string> paths)
{
    std::sort(paths.begin(), paths.end(), [](std::string a, std::string b) { return a < b; });
    std::string str;
    for (auto path : paths) {
        std::ifstream ifs(path);
        str.assign(std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>());
        
        std::string title = std::filesystem::path(path).stem();

        int id = new_chapter(title);
        m_db->set_body(id, str);
    }
}

int Novelest::new_chapter(std::string title)
{
    auto iter = m_store->append();
    auto row = *iter;
    row[m_columns.m_col_title] = title;
    row[m_columns.m_col_include] = true;
    int id = m_db->insert(title);
    row[m_columns.m_col_id] = id;
    return id;
}

void Novelest::on_new_chapter()
{
    new_chapter("New Chapter");
}

void Novelest::on_selection_changed()
{
    if (!m_editor->is_sensitive())
        m_editor->set_sensitive(true);
    if (m_current_iter) {
        std::string cur = m_editor->get_buffer()->get_text();
        int id = (*m_current_iter)[m_columns.m_col_id];
        m_db->set_body(id, cur);
    }
    try {
        m_editor->get_buffer()->set_text(
            m_db->get_body(m_selection->get_selected()->get_value(m_columns.m_col_id)));
    } catch (std::string e) {
        std::cout << e << std::endl;
    }
    m_current_iter = m_selection->get_selected();

    update_word_count();
}

void Novelest::on_include_toggled(Glib::ustring path)
{
    auto iter = m_store->get_iter(path);
    auto row = *iter;
    int id = row[m_columns.m_col_id];
    m_db->set_include(id, row[m_columns.m_col_include]);
    update_word_count();
}

void Novelest::on_chapter_title_edited(Glib::ustring path, Glib::ustring text)
{
    auto iter = m_store->get_iter(path);
    int id = (*iter)[m_columns.m_col_id];
    m_db->set_title(id, text);
}

std::string Novelest::get_save_filename()
{
    Gtk::FileChooserDialog dlg("Select a file name", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dlg.set_transient_for(*this);

    dlg.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("Open", Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name("Novelest files");
    filter->add_pattern("*.novelest");
    dlg.add_filter(filter);

    int result = dlg.run();

    switch (result) {
    case (Gtk::RESPONSE_CANCEL):
        return std::string {};
    case (Gtk::RESPONSE_OK):
        return dlg.get_filename();
    default:
        return std::string {};
    }
}

void Novelest::on_menu_save_as()
{
    std::string filename = get_save_filename();
    if (filename.empty())
        return;
    else {
        m_db = std::make_unique<Database>(filename);
    }
}

void Novelest::on_menu_open()
{
    Gtk::FileChooserDialog dlg("Select Novelest File", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_transient_for(*this);

    dlg.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("Open", Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name("Novelest Files");
    filter->add_pattern("*.novelest");
    dlg.add_filter(filter);

    int result = dlg.run();

    if (Gtk::RESPONSE_OK) {
        std::cout << dlg.get_filename() << std::endl;
        m_db = std::make_unique<Database>(dlg.get_filename());
        std::vector<Chapter> chapters = m_db->get_chapters();
        m_store->clear();
        for (auto i : chapters) {
            auto iter = m_store->append();
            auto row = *iter;
            row[m_columns.m_col_title] = i.title;
            row[m_columns.m_col_include] = i.include;
            row[m_columns.m_col_id] = i.id;
        }
    }
    dlg.close();
    update_word_count();
}

void Novelest::on_menu_new()
{
    Gtk::Dialog* dlg;
    m_builder->get_widget("dialog_new", dlg);

    dlg->add_button("Ok", Gtk::RESPONSE_OK);
    dlg->add_button("Cancel", Gtk::RESPONSE_CANCEL);

    Gtk::Entry* entry_title;
    m_builder->get_widget("new_dialog_title", entry_title);
    Gtk::Entry* entry_author;
    m_builder->get_widget("new_dialog_author", entry_author);
    Gtk::SpinButton* entry_target;
    m_builder->get_widget("new_dialog_target", entry_target);

    int response = dlg->run();

    if (Gtk::RESPONSE_OK) {
        std::string filename = get_save_filename();
        auto title = entry_title->get_text();
        auto author = entry_author->get_text();
        auto target = entry_target->get_value();
        if (!m_db)
            m_db = std::make_unique<Database>(filename, title, author, target);
        if (!m_btn_new_chapter)
            m_builder->get_widget("btn_new_chapter", m_btn_new_chapter);
        m_btn_new_chapter->set_sensitive(true);
        m_editor->set_sensitive(true);
        dlg->close();
    }
    update_word_count();
}

void Novelest::on_menu_import()
{
    Gtk::FileChooserDialog dlg("Select markdown file(s)", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dlg.set_select_multiple(true);
    dlg.add_choice("add", "Add to current project");
    dlg.set_transient_for(*this);
    dlg.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    dlg.add_button("Open", Gtk::RESPONSE_OK);

    auto filter = Gtk::FileFilter::create();
    filter->set_name("Markdown Files");
    filter->add_pattern("*.md");
    filter->add_pattern("*.markdown");
    dlg.add_filter(filter);

    int result = dlg.run();
    
    if (result == Gtk::RESPONSE_OK) {
        if (!m_db)
            m_db = std::make_unique<Database>(get_save_filename());
        import(dlg.get_filenames());
    }
    dlg.close();
}