using AForge.Video;
using AForge.Video.DirectShow;
using System;
using System.Drawing;

namespace NexLink_Tool.Page
{
    /// <summary>Captures frames from the first DirectShow video input device.</summary>
    public sealed class CameraFrameSource : IFrameSource
    {
        private readonly object _lock = new object();
        private VideoCaptureDevice _camera;
        private Bitmap _latestFrame;

        // Mirrored so the preview matches the user's own movement.
        public bool MirrorHorizontally => true;
        public bool CropToTargetAspect => true;

        /// <summary>Starts the camera. Returns null on success, otherwise the reason it failed.</summary>
        public string Start()
        {
            try
            {
                var cameras = new FilterInfoCollection(FilterCategory.VideoInputDevice);
                if (cameras.Count == 0)
                    return "No camera found";

                _camera = new VideoCaptureDevice(cameras[0].MonikerString);
                _camera.NewFrame += OnNewFrame;
                _camera.Start();
                return null;
            }
            catch (Exception ex)
            {
                Stop();
                return $"Camera failed: {ex.Message}";
            }
        }

        public Bitmap GetFrame()
        {
            lock (_lock)
                return _latestFrame == null ? null : (Bitmap)_latestFrame.Clone();
        }

        public void Stop()
        {
            if (_camera != null)
            {
                _camera.NewFrame -= OnNewFrame;
                if (_camera.IsRunning)
                    _camera.SignalToStop();
                _camera.WaitForStop();
                _camera = null;
            }

            lock (_lock)
            {
                _latestFrame?.Dispose();
                _latestFrame = null;
            }
        }

        private void OnNewFrame(object sender, NewFrameEventArgs e)
        {
            var frame = (Bitmap)e.Frame.Clone();
            lock (_lock)
            {
                _latestFrame?.Dispose();
                _latestFrame = frame;
            }
        }
    }
}
