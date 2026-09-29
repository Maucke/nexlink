using System.Drawing;

namespace NexLink_Tool.Page
{
    /// <summary>
    /// Produces frames for <see cref="StreamOverlay"/>. A source owns its capture resources;
    /// the overlay keeps responsibility for cropping, RGB565 encoding and the USB send loop.
    /// </summary>
    public interface IFrameSource
    {
        /// <summary>Whether the encoded frame should be flipped left-to-right.</summary>
        bool MirrorHorizontally { get; }

        /// <summary>Whether frames must be center-cropped to the display's aspect ratio.</summary>
        bool CropToTargetAspect { get; }

        /// <summary>Returns the next 24bpp frame, or null when none is available yet.</summary>
        Bitmap GetFrame();

        /// <summary>Releases capture resources. Safe to call more than once.</summary>
        void Stop();
    }
}
