#include "qtqq_server.h"
#include "qmessagebox.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSqlRecord>
#include <QSqlQuery>
#include <qfiledialog.h>
const int global_tcp_port = 6666;
const int global_udp_port = 8888;
QtqqServer::QtqqServer(QWidget* parent)
    : QDialog(parent),
	pic_path_("")
{
    ui.setupUi(this);

	if (!ConnectDatabase()) {
		QMessageBox::warning(NULL,QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("连接数据库失败！"));
		this->close();
		return;
	}
	SetDepNameMap();
	SetStatusMap();
	SetOnlineMap();

	InitComboBoxData();
	queryinfo_model_.setQuery("SELECT * FORM qt_qq.tab_employees");
	ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);	//表格只读

	//初始化查询公司群所有员工信息
	dep_id_ = GetComDepID();
	employee_id_ = 0;
	compony_depid_ = dep_id_;
	UpdateTableData();
	//定时刷新数据
	timer_ = new QTimer(this);
	timer_->setInterval(200);
	timer_->start();
	connect(timer_, &QTimer::timeout, this, &QtqqServer::OnRefresh);
	InitTcpSocket();
	InitUdpSocket();
}

QtqqServer::~QtqqServer()
{}
   
void QtqqServer::InitComboBoxData(){
	QString item_text;	//组合框项的文本
	QSqlQueryModel query_dep_model_; //获取公司总的部门
	query_dep_model_.setQuery("SELECT * FROM qt_qq.tab_department");
	int dep_count = query_dep_model_.rowCount() - 1;	//部门总数减去公司群 公司群不算公司部门
	for (int i = 0; i < dep_count; i++) {
		item_text = ui.employee_dep_box->itemText(i);
		QSqlQuery query_dep_id(QString("SELECT departmentID FROM qt_qq.tab_department WHERE department_name = '%1'").arg(item_text));
		query_dep_id.first();
		//设置当前组合框数据为相应的部门QQ号
		ui.employee_dep_box->setItemData(i, query_dep_id.value(0).toInt());
	}
	//公司群筛选	多一个公司群部门
	for (size_t i = 0; i < dep_count + 1; i++) {
		item_text = ui.department_box->itemText(i);
		QSqlQuery query_dep_id(QString("SELECT departmentID FROM qt_qq.tab_department WHERE department_name = '%1'").arg(item_text));
		query_dep_id.first();
		//设置部门组合框的数据为相应的部门QQ号
		ui.department_box->setItemData(i, query_dep_id.value(0).toInt());
	}
}

void QtqqServer::InitTcpSocket() {
    tcp_server_ = new TcpServer(global_tcp_port);
    tcp_server_->run();
    // 收到Tcp客户端发来的信息后进行udp 广播  
    connect(tcp_server_, &TcpServer::SignalTcpMsgComes, this, &QtqqServer::OnUdpBroadMsg);
}

void QtqqServer::InitUdpSocket(){
	udp_sender_ = new QUdpSocket(this);

}

void QtqqServer::OnUdpBroadMsg(QByteArray& bt_data) {
	for (qint16 port = global_udp_port; port < global_udp_port + 200; ++port){
		udp_sender_->writeDatagram(bt_data,bt_data.size(),QHostAddress::Broadcast,port);
	}
}

bool QtqqServer::ConnectDatabase()
{
	QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
	db.setDatabaseName("mysql"); //数据库名称  
	db.setHostName("localhost");
	db.setUserName("root");
	db.setPassword("123456");
	db.setPort(3306);

	if (db.open()) {
		return true;
	}
	else {
		return false;
	}
}

int QtqqServer::GetComDepID()
{
	QSqlQuery query_compdep_ID(QString("SELECT departmentID FROM qt_qq.tab_department WHERE department_name = '%1'").arg(QString::fromLocal8Bit("公司群")));
	query_compdep_ID.first();
	int com_Dep_id = query_compdep_ID.value(0).toInt();
	return com_Dep_id;
}

void QtqqServer::SetDepNameMap(){
	dep_name_map_.insert(QStringLiteral("2001"), QStringLiteral("人事群"));
	dep_name_map_.insert(QStringLiteral("2002"), QStringLiteral("研发群"));
	dep_name_map_.insert(QStringLiteral("2003"), QStringLiteral("市场群"));
}

void QtqqServer::SetStatusMap(){
	status_map_.insert(QStringLiteral("1"), QStringLiteral("有效"));
	status_map_.insert(QStringLiteral("0"), QStringLiteral("已注销"));
}

void QtqqServer::SetOnlineMap(){
	online_map_.insert(QStringLiteral("1"), QStringLiteral("离线"));
	online_map_.insert(QStringLiteral("2"), QStringLiteral("在线"));
	online_map_.insert(QStringLiteral("3"), QStringLiteral("隐身"));
}

void QtqqServer::UpdateTableData(int dep_id, int employ_id){
	ui.tableWidget->clear();
	if (dep_id && dep_id != compony_depid_) {
		queryinfo_model_.setQuery(QString("SELECT * FROM qt_qq.tab_employees WHERE departmentID = %1").arg(dep_id));
	}
	else if (employ_id) { //精确查找
		queryinfo_model_.setQuery(QString("SELECT * FROM qt_qq.tab_employees WHERE employeeID = %1").arg(employ_id));
	}
	else if (dep_id == compony_depid_){ //公司群
		queryinfo_model_.setQuery(QString("SELECT * FROM qt_qq.tab_employees"));
	}
	int rows = queryinfo_model_.rowCount();				//总行数（总记录数）
	int columns = queryinfo_model_.columnCount();		//总列数（总记录数）
	//模型索引
	QModelIndex index;
	//设置表格的行数与列数
	ui.tableWidget->setRowCount(rows);
	ui.tableWidget->setColumnCount(columns);

	//设置表头
	QStringList headers;
	headers << QStringLiteral("部门")
			<< QStringLiteral("工号")
			<< QStringLiteral("员工姓名")
			<< QStringLiteral("员工签名")
			<< QStringLiteral("员工状态")
			<< QStringLiteral("员工头像")
			<< QStringLiteral("在线状态");
	ui.tableWidget->setHorizontalHeaderLabels(headers);
	//设置列等宽
	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	for (int i = 0; i < rows; i++){
		for (int j = 0; j < columns; j++){
			//行，列
			index = queryinfo_model_.index(i, j);
			//获取 i行 j列的数据
			QString str_data = queryinfo_model_.data(index).toString();
			//获取字段名称
			QSqlRecord record = queryinfo_model_.record(i);	//当前行的记录  一整行
			QString str_record_name = record.fieldName(j);	//列
			if (str_record_name == QLatin1String("departmentID")) {
				ui.tableWidget->setItem(i,j,new QTableWidgetItem(dep_name_map_.value(str_data)));
				continue;
			}
			else if (str_record_name == QLatin1String("status")) {
				ui.tableWidget->setItem(i, j, new QTableWidgetItem(status_map_.value(str_data)));
				continue;
			}
			//此处online字段
			else if (str_record_name == QLatin1String("online")) {
				ui.tableWidget->setItem(i, j, new QTableWidgetItem(online_map_.value(str_data)));
				continue;
			}
			ui.tableWidget->setItem(i,j,new QTableWidgetItem(str_data));
		}
	}
}

void QtqqServer::OnRefresh(){
	UpdateTableData(compony_depid_, employee_id_);
}

void QtqqServer::on_query_department_btn_clicked(){
	ui.queryid_lineedit->clear();
	employee_id_ = 0;
	dep_id_ = ui.department_box->currentData().toInt();
	UpdateTableData(dep_id_);
}

void QtqqServer::on_queryid_btn_clicked(){
	ui.department_box->setCurrentIndex(0);
	dep_id_ = compony_depid_;
	if (!ui.queryid_lineedit->text().length()) {
		QMessageBox::information(this, QStringLiteral("提示"),
			QStringLiteral("请输入员工QQ号"));
		//焦点放入输入框
		ui.queryid_lineedit->setFocus();
		return;
	}
	//检测员工号是否输入正确
	int employee_id = ui.queryid_lineedit->text().toInt();

	//检测输入的员工QQ号的合法性
	QSqlQuery query_info(QString("SELECT * FROM qt_qq.tab_employees WHERE employeeID = %1").arg(employee_id));
	if (!query_info.first()) {
		QMessageBox::information(this, QStringLiteral("提示"),
			QStringLiteral("请输入正确的员工QQ号"));
		//焦点放入输入框
		ui.queryid_lineedit->setFocus();
		return;
	}
	else {
		employee_id_ = employee_id;
	}

}

void QtqqServer::on_logout_btn_clicked(){
	ui.queryid_lineedit->clear();
	ui.employee_dep_box->setCurrentIndex(0);

	if (!ui.logoutid_lineedit->text().length()) {
		QMessageBox::information(this, QStringLiteral("提示"),
			QStringLiteral("请输入员工QQ号"));
		//焦点放入输入框
		ui.logoutid_lineedit->setFocus();
		return;
	}
	//检测员工号是否输入正确
	int employee_id = ui.logoutid_lineedit->text().toInt();
	QSqlQuery query_info(QString("SELECT employee_name FROM qt_qq.tab_employees WHERE employeeID = %1").arg(employee_id));
	if (!query_info.first()) {
		QMessageBox::information(this, QStringLiteral("提示"),
			QStringLiteral("请输入正确的员工QQ号"));
		//焦点放入输入框
		ui.logoutid_lineedit->setFocus();
		return;
	}
	else {
		//注销操作，更新数据库数据，将员工的状态设置为0
		QSqlQuery sql_update(QString("UPDATE qt_qq.tab_employees SET status = 0 WHERE employeeID = %1").arg(employee_id));
		sql_update.first();

		//获取注销员工的姓名
		QString str_name = query_info.value(0).toString();
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("员工%1的QQ%2已经被注销").arg(str_name).arg(employee_id));
		ui.logoutid_lineedit->clear();
	}
}

void QtqqServer::on_select_picture_btn_clicked(){           //目录当前"."
	//获取选择的头像路径 
	pic_path_ = QFileDialog::getOpenFileName(this,QString::fromLocal8Bit("选择头像"),".","*.png;;*.jpg");
	if (!pic_path_.size()) {
		return;
	}
	//将头像显示在标签上面
	QPixmap pixmap;
	pixmap.load(pic_path_);
	qreal width_ratio = (qreal)ui.head_label->width()/(qreal)pixmap.width();
	qreal height_ratio = (qreal)ui.head_label->height() / (qreal)pixmap.height();
	QSize size(pixmap.width() * width_ratio, pixmap.height() * height_ratio);
	ui.head_label->setPixmap(pixmap.scaled(size));
}

void QtqqServer::on_add_btn_clicked(){
	//检测员工姓名的输入
	QString str_name = ui.name_line_edit->text();
	if (!str_name.size()) {
		QMessageBox::information(this,QString::fromLocal8Bit("提示"),QString::fromLocal8Bit("请输入员工姓名！"));
		ui.name_line_edit->setFocus();
		return;
	}
	//检测员工选择头像
	if (!pic_path_.size()) {
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("请选择员工头像路径！")); 
		return;
	}
	//数据库插入新的员工距离		
	//获取员工QQ号
	QSqlQuery max_employee_id("SELECT MAX(employeeID) FROM qt_qq.tab_employees");
	max_employee_id.first();
	int employee_id = max_employee_id.value(0).toInt() + 1;
	//员工部门QQ号
	int dep_id = ui.employee_dep_box->currentData().toInt();
	//图片路径设置为 ”/“ 替换为”\“     XXX\XXX\  XXX.png
	QString str_pixpath = pic_path_;
	//转义符和斜杠
	str_pixpath.replace("/", "\\\\");  

	QSqlQuery insert_sql(QString("INSERT INTO qt_qq.tab_employees(departmentID,employeeID,employee_name,picture)VALUES(%1,%2,'%3','%4')")
		.arg(dep_id).arg(employee_id).arg(str_name).arg(str_pixpath));
	insert_sql.first();
	QMessageBox::information(this,QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("新增员工成功"));
	pic_path_ = "";
	ui.name_line_edit->clear();
	ui.head_label->setText(QStringLiteral("  员工寸照   "));

}
