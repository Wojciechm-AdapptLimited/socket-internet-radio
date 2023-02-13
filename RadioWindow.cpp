#include "RadioWindow.h"

RadioWindow::RadioWindow() : skipButton("Skip"), addButton("Add"), removeButton("Remove"), reorderButton("Reorder") {
    set_title("Internet Radio");
    set_default_size(640, 480);
    set_border_width(10);

    add(buttonBox);

    buttonBox.set_border_width(5);
    buttonBox.set_layout(Gtk::BUTTONBOX_SPREAD);
    buttonBox.set_spacing(40);

    skipButton.signal_clicked().connect(sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &RadioWindow::onButtonClicked), "skip"));
    buttonBox.add(skipButton);

    addButton.signal_clicked().connect(sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &RadioWindow::onButtonClicked), "add"));
    buttonBox.add(addButton);

    removeButton.signal_clicked().connect(sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &RadioWindow::onButtonClicked), "remove"));
    buttonBox.add(removeButton);

    reorderButton.signal_clicked().connect(sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &RadioWindow::onButtonClicked), "reorder"));
    buttonBox.add(reorderButton);

    skipButton.show();
    addButton.show();
    removeButton.show();
    reorderButton.show();
    buttonBox.show();
}

void RadioWindow::onButtonClicked(const Glib::ustring& data) {
    std::cout << "Hello World from " << data << "\n";
}