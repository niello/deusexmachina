namespace DialogDesigner
{
    partial class FormDialogPlayer
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
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
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.Panel panel1;
            System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
            this._tableDialogs = new System.Windows.Forms.TableLayoutPanel();
            this._playerProgress = new System.Windows.Forms.ProgressBar();
            this._buttonRestart = new System.Windows.Forms.Button();
            this._buttonBack = new System.Windows.Forms.Button();
            this._buttonPlay = new System.Windows.Forms.Button();
            this._buttonNext = new System.Windows.Forms.Button();
            panel1 = new System.Windows.Forms.Panel();
            tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            panel1.SuspendLayout();
            tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            panel1.Controls.Add(this._tableDialogs);
            panel1.Controls.Add(this._playerProgress);
            panel1.Controls.Add(tableLayoutPanel1);
            panel1.Location = new System.Drawing.Point(13, 13);
            panel1.Name = "panel1";
            panel1.Size = new System.Drawing.Size(473, 364);
            panel1.TabIndex = 0;
            // 
            // _tableDialogs
            // 
            this._tableDialogs.ColumnCount = 2;
            this._tableDialogs.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this._tableDialogs.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this._tableDialogs.Dock = System.Windows.Forms.DockStyle.Fill;
            this._tableDialogs.Location = new System.Drawing.Point(0, 0);
            this._tableDialogs.Name = "_tableDialogs";
            this._tableDialogs.RowCount = 2;
            this._tableDialogs.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this._tableDialogs.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this._tableDialogs.Size = new System.Drawing.Size(473, 299);
            this._tableDialogs.TabIndex = 1;
            // 
            // _playerProgress
            // 
            this._playerProgress.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._playerProgress.Location = new System.Drawing.Point(0, 299);
            this._playerProgress.Name = "_playerProgress";
            this._playerProgress.Size = new System.Drawing.Size(473, 23);
            this._playerProgress.TabIndex = 2;
            // 
            // tableLayoutPanel1
            // 
            tableLayoutPanel1.ColumnCount = 6;
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 42F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 42F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 42F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 42F));
            tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            tableLayoutPanel1.Controls.Add(this._buttonRestart, 1, 0);
            tableLayoutPanel1.Controls.Add(this._buttonBack, 2, 0);
            tableLayoutPanel1.Controls.Add(this._buttonPlay, 3, 0);
            tableLayoutPanel1.Controls.Add(this._buttonNext, 4, 0);
            tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            tableLayoutPanel1.Location = new System.Drawing.Point(0, 322);
            tableLayoutPanel1.Name = "tableLayoutPanel1";
            tableLayoutPanel1.RowCount = 1;
            tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            tableLayoutPanel1.Size = new System.Drawing.Size(473, 42);
            tableLayoutPanel1.TabIndex = 0;
            // 
            // _buttonRestart
            // 
            this._buttonRestart.Image = global::DialogDesigner.Properties.Resources.media_beginning_32;
            this._buttonRestart.Location = new System.Drawing.Point(155, 3);
            this._buttonRestart.Name = "_buttonRestart";
            this._buttonRestart.Size = new System.Drawing.Size(36, 36);
            this._buttonRestart.TabIndex = 0;
            this._buttonRestart.UseVisualStyleBackColor = true;
            this._buttonRestart.Click += new System.EventHandler(this.ButtonRestartClick);
            // 
            // _buttonBack
            // 
            this._buttonBack.Image = global::DialogDesigner.Properties.Resources.media_rewind_32;
            this._buttonBack.Location = new System.Drawing.Point(197, 3);
            this._buttonBack.Name = "_buttonBack";
            this._buttonBack.Size = new System.Drawing.Size(36, 36);
            this._buttonBack.TabIndex = 1;
            this._buttonBack.UseVisualStyleBackColor = true;
            this._buttonBack.Click += new System.EventHandler(this.ButtonBackClick);
            // 
            // _buttonPlay
            // 
            this._buttonPlay.Image = global::DialogDesigner.Properties.Resources.media_play_32;
            this._buttonPlay.Location = new System.Drawing.Point(239, 3);
            this._buttonPlay.Name = "_buttonPlay";
            this._buttonPlay.Size = new System.Drawing.Size(36, 36);
            this._buttonPlay.TabIndex = 2;
            this._buttonPlay.UseVisualStyleBackColor = true;
            this._buttonPlay.Click += new System.EventHandler(this.ButtonPlayClick);
            // 
            // _buttonNext
            // 
            this._buttonNext.Image = global::DialogDesigner.Properties.Resources.media_fast_forward_32;
            this._buttonNext.Location = new System.Drawing.Point(281, 3);
            this._buttonNext.Name = "_buttonNext";
            this._buttonNext.Size = new System.Drawing.Size(36, 36);
            this._buttonNext.TabIndex = 3;
            this._buttonNext.UseVisualStyleBackColor = true;
            this._buttonNext.Click += new System.EventHandler(this.ButtonNextClick);
            // 
            // FormDialogPlayer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(498, 389);
            this.Controls.Add(panel1);
            this.Name = "FormDialogPlayer";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Dialog player";
            panel1.ResumeLayout(false);
            tableLayoutPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button _buttonBack;
        private System.Windows.Forms.Button _buttonPlay;
        private System.Windows.Forms.Button _buttonNext;
        private System.Windows.Forms.TableLayoutPanel _tableDialogs;
        private System.Windows.Forms.Button _buttonRestart;
        private System.Windows.Forms.ProgressBar _playerProgress;
    }
}