using System;
using System.Windows.Forms;
using CreatorIDE.Engine;

namespace CreatorIDE.Package
{
    public class EngineHostControl: Control
    {
        private const int AdvanceInteval = 40;

        private readonly Timer _timer = new Timer {Interval = AdvanceInteval};
        private DateTime _advanceTime = DateTime.Now;

        public event EventHandler Load;

        public CideEngine Engine { get; set; }

        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            OnLoad();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            var engine = Engine;
            if (engine != null)
            {
                var time = DateTime.Now;
                if ((time - _advanceTime).TotalMilliseconds >= AdvanceInteval)
                {
                    engine.Advance();
                    _advanceTime = time;
                }
            }
            base.OnPaint(e);
        }

        private void OnLoad()
        {
            var h = Load;
            if (h != null)
                h(this, EventArgs.Empty);

            _timer.Tick += OnTimerTick;
            _timer.Start();
        }

        private void OnTimerTick(object sender, EventArgs e)
        {
            _timer.Tick -= OnTimerTick;
            Invalidate();
            _timer.Tick += OnTimerTick;
        }

        protected override void Dispose(bool disposing)
        {
            _timer.Dispose();

            base.Dispose(disposing);
        }
    }
}
