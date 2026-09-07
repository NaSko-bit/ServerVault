import sys
from PySide6.QtUiTools import QUiLoader
from PySide6.QtWidgets import QApplication

#server button clicked function
def server_button_clicked():
    if(window.pushButton_2.text() == "Start Server"):
        window.pushButton_2.setText("Stop Server")
        window.label.setText("Server STATUS: ONLINE\nIP: 127.0.0.1\nPORT: 2000")
        print("Server Started")
    else:
        window.pushButton_2.setText("Start Server")
        window.label.setText("Server STATUS: OFFLINE\nIP:127.0.0.1\nPORT: 2000")
        print("Server Stopped")

#setup the application
app = QApplication([])
window = QUiLoader().load("MainWindow.ui")
window.show()
window.pushButton_2.clicked.connect(server_button_clicked)

#start the application
app.exec()