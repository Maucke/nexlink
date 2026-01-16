using System;

namespace NexLink
{
    public class NexLinkException : Exception
    {
        public NexLinkError Error { get; }

        public NexLinkException(
            NexLinkError error,
            string message)
            : base(message)
        {
            Error = error;
        }

        public NexLinkException(
            NexLinkError error,
            string message,
            Exception inner)
            : base(message, inner)
        {
            Error = error;
        }

        public override string ToString()
        {
            return $"NexLinkError={Error}: {Message}";
        }
    }
}
