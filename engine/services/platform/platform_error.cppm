export module ember.platform:platform_error;

export namespace ember::platform
{

/// Reasons why a platform resource cannot be created.
enum class PlatformError
{
    /// The window or its graphics context cannot be created, for example because there is
    /// no display.
    WindowUnavailable,
    /// No audio device can be opened.
    AudioUnavailable,
    /// The data is corrupt or has a format that is not supported.
    InvalidData
};

} // namespace ember::platform
