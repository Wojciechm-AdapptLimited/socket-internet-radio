#include <gtkmm.h>
#include <iostream>

#ifndef INTERNET_RADIO_RADIOWINDOW_H
#define INTERNET_RADIO_RADIOWINDOW_H


class RadioWindow : public Gtk::Window {
private:
    Gtk::Box buttonBox;
    Gtk::Button skipButton, addButton, removeButton, reorderButton;

    void onButtonClicked(const Glib::ustring& data);
public:
    RadioWindow();
};


#endif //INTERNET_RADIO_RADIOWINDOW_H
