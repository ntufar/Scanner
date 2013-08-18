#include <gtkmm.h>
#include <iostream>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "telnet.h"
#ifdef POSIX
#include <termios.h>
#endif

using boost::asio::ip::tcp;
using namespace std;

class ScannerGUI : public Gtk::Window
{

public:
  ScannerGUI();
  virtual ~ScannerGUI();
  void serverCommunicationThread();

protected:
  //Signal handlers:
  void on_button_clicked();

  //Member widgets:
  Gtk::Box m_box1;
  Gtk::Button m_button1;
  Gtk::TextView m_TextView;  
  Gtk::ScrolledWindow m_ScrolledWindow;
  Glib::RefPtr<Gtk::TextBuffer> m_refTextBuffer1;
  Glib::Thread * m_thread; 
  
  Gtk::Button m_button2;
  Gtk::ProgressBar m_progress;
};

void ScannerGUI::serverCommunicationThread() 
{ 
		// on Unix POXIS based systems, turn off line buffering of input, so cin.get() returns after every keypress
		// On other systems, you'll need to look for an equivalent
		#ifdef POSIX
			termios stored_settings;
			tcgetattr(0, &stored_settings);
			termios new_settings = stored_settings;
			new_settings.c_lflag &= (~ICANON);
			new_settings.c_lflag &= (~ISIG); // don't automatically handle control-C
			tcsetattr(0, TCSANOW, &new_settings);
		#endif
		try{
			boost::asio::io_service io_service;
			// resolve the host name and port number to an iterator that can be used to connect to the server
			tcp::resolver resolver(io_service);
			
			tcp::resolver::query query("localhost", "65001");
			tcp::resolver::iterator iterator = resolver.resolve(query);
			// define an instance of the main class of this program
			telnet_client c(io_service, iterator);
			// run the IO service as a separate thread, so the main thread can block on standard input
			boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));	
			while (1)
			{
				string ch;
				while(!c.readque.empty()){
					ch += c.readque.front();
					c.readque.pop_front();
				}
				cerr << ch;
				m_refTextBuffer1->insert_at_cursor(ch);
				Glib::usleep(1000);
				/*
				cerr << "DaTa AvAiLaBle"<<endl;
//				c.telnetin.getline(&ch,1024);
				cout << ch;
				cerr << ch;
				m_refTextBuffer1->insert_at_cursor(ch);
				* */
				/*
				cin.get(ch); // blocking wait for standard input
				if (ch == 3) // ctrl-C to end program
					break;
				c.write(ch);
				*/
			}
			c.close(); // close the telnet client connection
			t.join(); // wait for the IO service thread to close			
		}catch(exception& e){
				cerr << "Exception: " << e.what() << "\n";
		}
		#ifdef POSIX // restore default buffering of standard input
			tcsetattr(0, TCSANOW, &stored_settings);
		#endif		
		/*
  while(1) 
  { 
    Glib::usleep(50000);  // 50.000 Mikrosekunden warten 
    
    m_dispatcher();       // Threadsafer Aufruf der pulse()-Methode 
  
    // Dies ist ein anonymer scope [Erklärung siehe Destruktor Defintion] 
    { 
      Glib::Mutex::Lock lock(mutex); 
      if(m_end_thread)      // Müssen wir beenden? 
        return; 
    }// Ende des anonymen scopes, das den effekt hat das das mutex wir unlocked wird 
  }
  * */ 
}


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
  //m_refTextBuffer1->set_text("This is the text from TextBuffer #1.");
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

	m_thread = Glib::Thread::create( sigc::mem_fun(*this,&ScannerGUI::serverCommunicationThread),false); 

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
