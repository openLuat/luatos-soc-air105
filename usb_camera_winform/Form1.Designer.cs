namespace Air105摄像头预览
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.pictureBox_main = new System.Windows.Forms.PictureBox();
            this.label_comName = new System.Windows.Forms.Label();
            this.button_power = new System.Windows.Forms.Button();
            this.label_data_log = new System.Windows.Forms.Label();
            this.comboBox_coms = new System.Windows.Forms.ComboBox();
            this.button2 = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.textBox_br = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox_main)).BeginInit();
            this.SuspendLayout();
            // 
            // pictureBox_main
            // 
            this.pictureBox_main.BackColor = System.Drawing.SystemColors.ControlDark;
            this.pictureBox_main.Location = new System.Drawing.Point(12, 96);
            this.pictureBox_main.Name = "pictureBox_main";
            this.pictureBox_main.Size = new System.Drawing.Size(640, 480);
            this.pictureBox_main.TabIndex = 0;
            this.pictureBox_main.TabStop = false;
            // 
            // label_comName
            // 
            this.label_comName.AutoSize = true;
            this.label_comName.Location = new System.Drawing.Point(12, 14);
            this.label_comName.Name = "label_comName";
            this.label_comName.Size = new System.Drawing.Size(39, 20);
            this.label_comName.TabIndex = 2;
            this.label_comName.Text = "串口";
            this.label_comName.Click += new System.EventHandler(this.label_comName_Click);
            // 
            // button_power
            // 
            this.button_power.BackColor = System.Drawing.Color.Green;
            this.button_power.Location = new System.Drawing.Point(259, 49);
            this.button_power.Name = "button_power";
            this.button_power.Size = new System.Drawing.Size(94, 29);
            this.button_power.TabIndex = 3;
            this.button_power.Text = "开始读取";
            this.button_power.UseVisualStyleBackColor = false;
            this.button_power.Click += new System.EventHandler(this.button1_Click);
            // 
            // label_data_log
            // 
            this.label_data_log.AutoSize = true;
            this.label_data_log.Location = new System.Drawing.Point(13, 604);
            this.label_data_log.Name = "label_data_log";
            this.label_data_log.Size = new System.Drawing.Size(69, 20);
            this.label_data_log.TabIndex = 4;
            this.label_data_log.Text = "数据日志";
            // 
            // comboBox_coms
            // 
            this.comboBox_coms.FormattingEnabled = true;
            this.comboBox_coms.Location = new System.Drawing.Point(92, 12);
            this.comboBox_coms.Name = "comboBox_coms";
            this.comboBox_coms.Size = new System.Drawing.Size(151, 28);
            this.comboBox_coms.TabIndex = 5;
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(259, 10);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(94, 29);
            this.button2.TabIndex = 6;
            this.button2.Text = "刷新串口";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 49);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(54, 20);
            this.label1.TabIndex = 7;
            this.label1.Text = "波特率";
            // 
            // textBox_br
            // 
            this.textBox_br.Location = new System.Drawing.Point(92, 49);
            this.textBox_br.Name = "textBox_br";
            this.textBox_br.Size = new System.Drawing.Size(151, 27);
            this.textBox_br.TabIndex = 8;
            this.textBox_br.Text = "1500000";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.ClientSize = new System.Drawing.Size(666, 719);
            this.Controls.Add(this.textBox_br);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.comboBox_coms);
            this.Controls.Add(this.label_data_log);
            this.Controls.Add(this.button_power);
            this.Controls.Add(this.label_comName);
            this.Controls.Add(this.pictureBox_main);
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "LuatOS摄像头预览";
            this.Load += new System.EventHandler(this.Form1_Load);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox_main)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private PictureBox pictureBox_main;
        private Label label_comName;
        private Button button_power;
        private Label label_data_log;
        private ComboBox comboBox_coms;
        private Button button2;
        private Label label1;
        private TextBox textBox_br;
    }
}