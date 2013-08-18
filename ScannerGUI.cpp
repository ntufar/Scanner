#include <gtkmm.h>
#include <iostream>


class ScannerGUI : public Gtk::Window
{

public:
  ScannerGUI();
  virtual ~ScannerGUI();

protected:
  //Signal handlers:
  void on_button_clicked();

  //Member widgets:
  Gtk::Box m_box1;
  Gtk::Button m_button1;
  Gtk::TextView m_TextView;  
  Gtk::ScrolledWindow m_ScrolledWindow;
  Glib::RefPtr<Gtk::TextBuffer> m_refTextBuffer1;
  
  Gtk::Button m_button2;
  Gtk::ProgressBar m_progress;
};



int main (int argc, char *argv[])
{
  Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

  ScannerGUI scannerGui;

  //Shows the window and returns when it is closed.
  return app->run(scannerGui);
}

ScannerGUI::ScannerGUI():
 	m_button1("Select folder to scan"),   // creates a new button with label "Hello World".
	m_button2("hello"),
	m_box1(Gtk::ORIENTATION_VERTICAL)
{
  // Sets the border width of the window.
  set_border_width(20);
  set_title("Scanner (c) Nicolai Tufar, 2013");
  set_position(Gtk::WindowPosition(Gtk::WIN_POS_CENTER));
  set_default_size(600, 400);
  
  
  

  // When the button receives the "clicked" signal, it will call the
  // on_button_clicked() method defined below.
  m_button1.signal_clicked().connect(sigc::mem_fun(*this,
              &ScannerGUI::on_button_clicked));

  m_box1.set_spacing(10);
  add(m_box1);
  m_box1.pack_start(m_button1, Gtk::PACK_SHRINK);
  m_button1.show();
  
  m_ScrolledWindow.add(m_TextView);
  m_refTextBuffer1 = Gtk::TextBuffer::create();
  m_refTextBuffer1->set_text("This is the text from TextBuffer #1.");
  m_TextView.set_buffer(m_refTextBuffer1);
  m_TextView.show();
  m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  
  m_box1.pack_start(m_ScrolledWindow);
  m_ScrolledWindow.show();
  
  //m_box1.pack_start(m_button2);
  //m_button2.show();
  m_box1.pack_start(m_progress, Gtk::PACK_SHRINK);
  //m_progress.set_default_size(570, 50);
  m_progress.set_text("Scanning progress");
  m_progress.set_show_text(true);
  m_progress.set_fraction(0.33);

  m_progress.show();

  m_box1.show();
}

ScannerGUI::~ScannerGUI()
{
}

void ScannerGUI::on_button_clicked()
{
 
  Gtk::FileChooserDialog dialog("Please choose a folder",
          Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_transient_for(*this);
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", Gtk::RESPONSE_OK);
    
  int result = dialog.run();

 switch(result)
  {
    case(Gtk::RESPONSE_OK):
    {
      std::cout << "Scanning: " << dialog.get_filename()
          << std::endl;
      break;
    }
    case(Gtk::RESPONSE_CANCEL):
    {
      std::cout << "Cancel clicked." << std::endl;
      break;
    }
    default:
    {
      std::cout << "Unexpected button clicked." << std::endl;
      break;
    }
  }
}
