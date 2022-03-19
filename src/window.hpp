#include <gtkmm.h>
#include <gtksourceviewmm/view.h>
#include <memory>
#include <string>

#include "db.hpp"
#include "util.hpp"

class Novelest : public Gtk::Window {
public:
    Novelest(Glib::RefPtr<Gtk::Application>);

protected:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns()
        {
            add(m_col_title);
            add(m_col_include);
            add(m_col_id);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_title;
        Gtk::TreeModelColumn<bool> m_col_include;
        Gtk::TreeModelColumn<guint> m_col_id;
    };

    ModelColumns m_columns;
    Gtk::Grid* m_grid;
    Glib::RefPtr<Gtk::Builder> m_builder;
    Glib::RefPtr<Gtk::ListStore> m_store;
    Glib::RefPtr<Gtk::TreeSelection> m_selection;
    Gtk::TreeIter m_current_iter;

    // Member Widgets
    Gtk::TreeView* m_treeview;
    Gsv::View* m_editor;
    Gtk::Button* m_btn_new_chapter;
    Gtk::ProgressBar* m_progress_word_count;

    Glib::RefPtr<Gtk::Application> m_app;

    std::unique_ptr<Database> m_db;

private:
    guint counter = 1;

    bool init_builder();
    void init_workspace();
    void init_store();
    void init_handlers();
    void init_menu();

    void update_word_count();

    // Signal Handlers
    void on_new_chapter();
    void on_selection_changed();
    void on_include_toggled(Glib::ustring);
    void on_chapter_title_edited(Glib::ustring, Glib::ustring);

    // Menu Handlers
    void on_menu_quit() { this->m_app->quit(); }
    void on_menu_save_as();
    std::string get_save_filename();
    void on_menu_open();
    void on_menu_new();
};