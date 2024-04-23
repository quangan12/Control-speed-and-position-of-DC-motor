using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using ZedGraph;

namespace testgui
{
    public partial class Form1 : Form
    {
        string SDatas = String.Empty; // Khai báo chuỗi để lưu dữ liệu cảm biến gửi qua Serial
        string SRealTime = String.Empty; // Khai báo chuỗi để lưu thời gian gửi qua Serial
        int status = 0; // Khai báo biến để xử lý sự kiện vẽ đồ thị
        double realtime = 0; //Khai báo biến thời gian để vẽ đồ thị
        double datas = 0; //Khai báo biến dữ liệu cảm biến để vẽ đồ thị
        int tickStart = 0;
        int time = 0;
        int stoptime = 0, setpoint = 0;
        int setdothi = 0;
        //int setpoint = 0;
        public Form1()
        {
            InitializeComponent();
        }
        
        private void Form1_Load(object sender, EventArgs e)
        {
            GraphPane dothi = zedGraphControl1.GraphPane;
            dothi.Title.Text = "Do thi";
            dothi.XAxis.Title.Text = "Thoi gian (ms)";
            dothi.YAxis.Title.Text = "rpm";
                
            RollingPointPairList list1 = new RollingPointPairList(60000);
            RollingPointPairList list2 = new RollingPointPairList(60000);
            
            LineItem duongline1 = dothi.AddCurve("Van toc", list1, Color.Red, SymbolType.None);
            LineItem duongline2 = dothi.AddCurve("SetPoint", list2, Color.Blue, SymbolType.None);

            dothi.XAxis.Scale.Min = 0;
            dothi.XAxis.Scale.Max = 5000;
            dothi.XAxis.Scale.MinorStep = 500;
            dothi.XAxis.Scale.MajorStep = 500;

            dothi.YAxis.Scale.Min = -10;
            dothi.YAxis.Scale.Max = 40;
            dothi.YAxis.Scale.MinorStep = 10;
            dothi.YAxis.Scale.MajorStep = 10;

            zedGraphControl1.AxisChange();
            zedGraphControl1.Invalidate();

            string[] myport = SerialPort.GetPortNames();
            selectCOM.Items.AddRange(myport);
            serialPort1.BaudRate = 115200;
            serialPort1.Parity = System.IO.Ports.Parity.None;
            serialPort1.DataBits = 8;
            serialPort1.StopBits = System.IO.Ports.StopBits.One;

        }
        private void Draw()
        {
            if (zedGraphControl1.GraphPane.CurveList.Count <= 0)
                return;
            LineItem duongline1 = zedGraphControl1.GraphPane.CurveList[0] as LineItem;
            LineItem duongline2 = zedGraphControl1.GraphPane.CurveList[1] as LineItem;
            if (duongline1 == null)
                return;
            if (duongline2 == null)
                return;
            IPointListEdit list1 = duongline1.Points as IPointListEdit;
            IPointListEdit list2 = duongline2.Points as IPointListEdit;
            if (list1 == null)
                return;
            if (list2 == null)
                return;
            //thay realtime  = time;
            list1.Add(time, datas); // Thêm điểm trên đồ thị
            list2.Add(time, setpoint); // Thêm điểm trên đồ thị
            Scale xScale = zedGraphControl1.GraphPane.XAxis.Scale;
            Scale yScale = zedGraphControl1.GraphPane.YAxis.Scale;
            // Tự động Scale theo trục x
            if (time > xScale.Max /*- xScale.MajorStep*/)
            {
                xScale.Max = time + xScale.MajorStep;
                //xScale.Min = xScale.Max - 30;
            }
            // Tự động Scale theo trục y
            if (datas > yScale.Max - yScale.MajorStep || setpoint > yScale.Max - yScale.MajorStep)
            {
                if(setpoint < datas)
                yScale.Max = datas + yScale.MajorStep;
                else yScale.Max = setpoint + yScale.MajorStep;
                yScale.MajorStep = (yScale.Max - yScale.Min) / 5;
            }
            else if (datas < yScale.Min + yScale.MajorStep || setpoint < yScale.Min + yScale.MajorStep)
            {
                if(setpoint > datas)
                yScale.Min = datas - yScale.MajorStep;
                else yScale.Min = setpoint + yScale.MajorStep;
                yScale.MajorStep = (yScale.Max - yScale.Min) / 5;
            }
            zedGraphControl1.AxisChange();
            zedGraphControl1.Invalidate();
            zedGraphControl1.Refresh();
        }
        // Xóa đồ thị, với ZedGraph thì phải khai báo lại như ở hàm Form1_Load, nếu không sẽ không hiển thị
        private void ClearZedGraph()
        {
            zedGraphControl1.GraphPane.CurveList.Clear(); // Xóa đường
            zedGraphControl1.GraphPane.GraphObjList.Clear(); // Xóa đối tượng

            zedGraphControl1.AxisChange();
            zedGraphControl1.Invalidate();

            GraphPane dothi = zedGraphControl1.GraphPane;
            dothi.Title.Text = "Do thi alo alo alo alo";
            dothi.XAxis.Title.Text = "Thoi gian";
            dothi.YAxis.Title.Text = "rpm";
            RollingPointPairList list1 = new RollingPointPairList(60000);
            RollingPointPairList list2 = new RollingPointPairList(60000);
            LineItem duongline1 = dothi.AddCurve("Van toc", list1, Color.Red, SymbolType.None);
            LineItem duongline2 = dothi.AddCurve("SetPoint", list2, Color.Blue, SymbolType.None);

            dothi.XAxis.Scale.Min = 0;
            dothi.XAxis.Scale.Max = 5000;
            dothi.XAxis.Scale.MinorStep = 500;
            dothi.XAxis.Scale.MajorStep = 500;

            dothi.YAxis.Scale.Min = -20;
            dothi.YAxis.Scale.Max = 40;
            dothi.YAxis.Scale.MinorStep = 10;
            dothi.YAxis.Scale.MajorStep = 10;

            zedGraphControl1.AxisChange();
            zedGraphControl1.Invalidate();
            zedGraphControl1.Refresh();
        }
        // Hàm xóa dữ liệu
        private void ResetValue()
        {
            realtime = 0;
            datas = 0;
            SDatas = String.Empty;
            SRealTime = String.Empty;
            time = 0;
            status = 0; // Chuyển status về 0
        }
        private void Data_Listview()
        {
            if (status == 0)
                return;
            else
            {
                ListViewItem item = new ListViewItem(time.ToString()); // Gán biến realtime vào cột đầu tiên của ListView
                item.SubItems.Add(datas.ToString());
                listView1.Items.Add(item); // Gán biến datas vào cột tiếp theo của ListView
                listView1.Items[listView1.Items.Count - 1].EnsureVisible(); // Hiện thị dòng được gán gần nhất ở ListView, tức là mình cuộn ListView theo dữ liệu gần nhất đó
            }
        }
       
        private void label1_Click(object sender, EventArgs e)
        {

        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
           
        }
        //nút Connect
        private void button1_Click(object sender, EventArgs e)
        {
            try
            {
                serialPort1.PortName = selectCOM.Text;
                serialPort1.Open();
                progressBar1.Value = 100;
                lb_connect.Text = "Connected";
                btnconnect.Enabled = false;
                btndisconnect.Enabled = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }
        //nút nhấn Start
        private void button1_Click_1(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                serialPort1.Write("Chay;");   
                timer1.Start();
                lb_start.Text = "Starting";
            }
            else
                MessageBox.Show("Bạn không thể chạy khi chưa kết nối với thiết bị", "Thông báo", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private void btndisconnect_Click(object sender, EventArgs e)
        {
            try
            {
                progressBar1.Value = 0;
                serialPort1.Close();
                lb_connect.Text = "Disconnected";
                btnconnect.Enabled = true;
                btndisconnect.Enabled = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void btnreset_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                DialogResult traloi;
                traloi = MessageBox.Show("Bạn có chắc muốn xóa?", "Xóa dữ liệu", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning);
                if (traloi == DialogResult.OK)
                {
                    if (serialPort1.IsOpen)
                    {
                        serialPort1.Write("Reset;"); //Gửi ký tự "R" qua Serial
                        serialPort1.Write("Reset;");
                        listView1.Items.Clear(); // Xóa listview
                        time = 0;
                        timer1.Stop();
                        ClearZedGraph();//Xóa đường trong đồ thị
                        ResetValue();//Xóa dữ liệu trong Form
                        lb_load.Text = "Not Loaded";
                    }
                    else
                        MessageBox.Show("Bạn không thể dừng khi chưa kết nối với thiết bị", "Thông báo", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
                MessageBox.Show("Bạn không thể xóa khi chưa kết nối với thiết bị", "Thông báo", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private void serialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                string rcv_data = serialPort1.ReadLine();
                //string SDatas = serialPort1.ReadExisting();
                if(rcv_data.Contains("|"))
                {
                    string[] part = rcv_data.Split('|');// Đọc một dòng của Serial, cắt chuỗi khi gặp ký tự gạch đứng
                    SRealTime = part[0];// Chuỗi đầu tiên lưu vào SRealTime
                    SDatas = part[1];// Chuỗi thứ hai lưu vào SDatas
                } 
                //SDatas = rcv_data;
                double.TryParse(SDatas, out datas); // Chuyển đổi sang kiểu double
                double.TryParse(SRealTime, out realtime);
                realtime = realtime / 1000.0; // Đối ms sang s
                status = 1; // Bắt sự kiện xử lý xong chuỗi, đổi status về 1 để hiển thị dữ liệu trong ListView và vẽ đồ thị
                //Invoke(new MethodInvoker(() => listBox1.Items.Add(SRealTime)));
                //Data_Listview();
                //Draw();
            }
            catch
            {
                return;
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (!serialPort1.IsOpen)
            {
                progressBar1.Value = 0;
            }
            else if (serialPort1.IsOpen)
            {
                if (time < stoptime)
                {
                    time += 20;
                    Draw();
                    Data_Listview();
                    zedGraphControl1.AxisChange();
                    zedGraphControl1.Invalidate();
                    zedGraphControl1.Refresh();
                    status = 0;
                    lb_start.Text = "STOPPED";
                }
                else
                {
                    for(int i =0;i<5;i++)
                    {
                        serialPort1.Write("Reset;");
                        Task.Delay(5);
                    }                  
                    time = 0;
                    status = 0;
                    timer1.Stop();
                    
                }
            }
        }

        private void selectPositon_SelectedIndexChanged(object sender, EventArgs e)
        {
            if(SelectMode.SelectedItem.ToString() == "SPEED")
            {
                label11.Text = "CONTROLLER DC MOTOR SPEED";
                serialPort1.Write("Selectmode=1.0;");
                setdothi = 0;
            }
            else
            {
                label11.Text = "CONTROLLER DC MOTOR POSITION";
                serialPort1.Write("Selectmode=2.0;");
                setdothi = 1;
            }
        }

        private void btnset_Click(object sender, EventArgs e)
        {          
            stoptime = int.Parse(txt_stoptime.Text);
            setpoint = int.Parse(txt_setpoint.Text);
            StringBuilder datasend = new StringBuilder();
            datasend.Append("Ksetpoint=");
            datasend.Append(txt_setpoint.Text);
            datasend.Append("\n");
            /*datasend.Append("Kp=");
            datasend.Append(txt_Kp.Text);
            datasend.Append("\n");
            datasend.Append("Ki=");
            datasend.Append(txt_Ki.Text);
            datasend.Append("\n");
            datasend.Append("Kd=");
            datasend.Append(txt_Kd.Text);*/
            datasend.Append(";");
            string sendtostm = datasend.ToString();
            serialPort1.Write(sendtostm);
            lb_load.Text = "LOADED";
        }
    }
}
