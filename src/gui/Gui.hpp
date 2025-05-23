#pragma once

#include <any>
#include <chrono>
#include <filesystem>
#include <format>
#include <functional>
#include <optional>
#include <print>
#include <span>
#include <typeindex>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <string>
#include <type_traits>
#include <utility>

#include <stb_image.h>

inline void glfw_error_callback(int error, const char* description)
{
    std::print(stderr, "[ERROR] GLFW Error ({}): {}\n", error, description);
}

namespace Gui
{

constexpr static auto GLSL_VERSION = "#version 330";

[[maybe_unused]] inline static ImFont* regular_font;
inline static ImFont* bold_font;

constexpr static auto hex_colour_to_imvec4(const int hexValue, const float alpha = 1.0F) -> ImVec4
{
    auto r = static_cast<float>(((hexValue >> 16) & 0xFF)) / 255.0F;
    auto g = static_cast<float>(((hexValue >> 8) & 0xFF)) / 255.0F;
    auto b = static_cast<float>((hexValue & 0xFF)) / 255.0F;
    return { r, g, b, alpha };
}

constexpr static void black_and_red_style()
{
    constexpr static auto BG_COLOUR = Gui::hex_colour_to_imvec4(0x181818);

    constexpr static auto ACCENT         = Gui::hex_colour_to_imvec4(0xE63946);
    constexpr static auto ACCENT_HOVERED = Gui::hex_colour_to_imvec4(0xD62828);
    constexpr static auto ACCENT_ACTIVE  = Gui::hex_colour_to_imvec4(0xFF4C4C);

    auto& style             = ImGui::GetStyle();
    style.WindowRounding    = 5.3F;
    style.FrameRounding     = 2.3F;
    style.ScrollbarRounding = 0;

    style.Colors[ImGuiCol_FrameBg] = BG_COLOUR;
    style.Colors[ImGuiCol_ChildBg] = BG_COLOUR;

    style.Colors[ImGuiCol_TitleBg]       = ACCENT;
    style.Colors[ImGuiCol_TitleBgActive] = ACCENT_ACTIVE;

    style.Colors[ImGuiCol_Header]        = ACCENT;
    style.Colors[ImGuiCol_HeaderHovered] = ACCENT_HOVERED;
    style.Colors[ImGuiCol_HeaderActive]  = ACCENT_ACTIVE;

    style.Colors[ImGuiCol_Button]        = ACCENT;
    style.Colors[ImGuiCol_ButtonHovered] = ACCENT_HOVERED;
    style.Colors[ImGuiCol_ButtonActive]  = ACCENT_ACTIVE;

    style.Colors[ImGuiCol_TableHeaderBg] = ACCENT_ACTIVE;
}

void load_default_fonts(const float regular_size = 14.0F, const float bold_size = 16.0F);

enum class ChildFlags : std::uint8_t
{
    None   = 0,
    Border = 1 << 0,
};

[[nodiscard]] constexpr static auto operator|(ChildFlags lhs, ChildFlags rhs) -> ChildFlags
{
    return static_cast<ChildFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

enum class WindowFlags : std::uint16_t
{
    None                    = 0,
    NoTitleBar              = 1 << 0,
    NoResize                = 1 << 1,
    NoMove                  = 1 << 2,
    NoScrollbar             = 1 << 3,
    NoCollapse              = 1 << 5,
    NoSavedSettings         = 1 << 8,
    AlwaysVerticalScrollbar = 1 << 14,
    NoDecoration            = NoTitleBar | NoResize | NoScrollbar | NoCollapse,
};

[[nodiscard]] constexpr static auto operator|(WindowFlags lhs, WindowFlags rhs) -> WindowFlags
{
    return static_cast<WindowFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

[[nodiscard]] auto init_window(const std::string& title, const int width, const int height)
  -> std::optional<GLFWwindow*>;

void shutdown(GLFWwindow* window);

template<std::invocable Callback>
void child(const std::string& title, ChildFlags child_flags, WindowFlags window_flags, Callback&& callback)
{
    if (ImGui::BeginChild(
          title.c_str(), ImVec2(0, 0), std::to_underlying(child_flags), std::to_underlying(window_flags)
        )) {
        std::invoke(std::forward<Callback>(callback));
    }

    ImGui::EndChild();
}

template<std::invocable Callback>
void child(
  const std::string& title,
  const ImVec2&      size,
  ChildFlags         child_flags,
  WindowFlags        window_flags,
  Callback&&         callback
)
{
    if (ImGui::BeginChild(title.c_str(), size, std::to_underlying(child_flags), std::to_underlying(window_flags))) {
        std::invoke(std::forward<Callback>(callback));
    }

    ImGui::EndChild();
}

template<std::invocable Callback>
void window(const std::string& title, WindowFlags window_flags, Callback&& callback)
{
    ImGui::Begin(title.c_str(), nullptr, std::to_underlying(window_flags));
    std::invoke(std::forward<Callback>(callback));
    ImGui::End();
}

void new_frame();


template<typename... Args>
void text(std::format_string<Args...> fmt, Args&&... args)
{
    ImGui::TextUnformatted(std::format(fmt, std::forward<Args>(args)...).c_str());
}

template<std::invocable Callback>
void group(Callback&& callback)
{
    ImGui::BeginGroup();
    std::invoke(std::forward<Callback>(callback));
    ImGui::EndGroup();
}

template<typename Callback>
void title(const std::string& title, const ImVec2& child_size, Callback&& callback)
  requires(std::invocable<Callback> || std::invocable<Callback, ImVec2>)
{
    constexpr static auto title_height = 24.0F;
    const auto            title_size   = ImVec2(child_size.x, title_height);

    Gui::group([&] {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
        Gui::child(std::format("{}_title", title), title_size, ChildFlags::None, WindowFlags::None, [&] {
            ImGui::PushFont(bold_font);

            ImGui::SetCursorPosX(8.0F);
            ImGui::SetCursorPosY((title_height - ImGui::GetTextLineHeight()) * 0.5F);
            Gui::text("{}", title);

            ImGui::PopFont();
        });
        ImGui::PopStyleColor();

        const auto spacing        = ImGui::GetStyle().ItemSpacing.y;
        const auto remaining_size = ImVec2(child_size.x, child_size.y - title_height - spacing);
        Gui::child(
          std::format("{}_Child", title),
          remaining_size,
          ChildFlags::Border,
          WindowFlags::None,
          [callback = std::forward<Callback>(callback), &remaining_size] {
              if constexpr (std::invocable<Callback, ImVec2>) {
                  std::invoke(std::move(callback), remaining_size);
              } else {
                  std::invoke(std::move(callback));
              }
          }
        );
    });
}

class [[nodiscard]] Texture final
{
  public:
    static auto load_from_file(const std::filesystem::path& path) -> Texture
    {
        int           width    = -1;
        int           height   = -1;
        int           channels = -1;
        std::uint8_t* bytes    = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
        if (bytes == nullptr) {
            std::println(stderr, "[ERROR] (stb) Failed to load file: {}", path.string());
            return Texture(std::nullopt);
        }

        GLuint texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(bytes);

        return Texture(texture_id);
    }

    [[nodiscard]] auto loaded() const -> bool { return texture_id.has_value(); }

    [[nodiscard]] auto id() const -> GLuint { return texture_id.value(); }

    [[nodiscard]] auto as_imgui_texture() const -> ImTextureID
    {
        return reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(texture_id.value()));
    }

    ~Texture()
    {
        if (texture_id.has_value()) { glDeleteTextures(1, &texture_id.value()); }
    }
    Texture(const Texture&)                = delete;
    Texture& operator=(const Texture&)     = delete;
    Texture(Texture&&) noexcept            = default;
    Texture& operator=(Texture&&) noexcept = default;

  private:
    explicit Texture(const std::optional<GLuint> texture_id_)
      : texture_id { texture_id_ }
    {}

    std::optional<GLuint> texture_id;
};

template<typename... Args>
void tooltip(const std::format_string<Args...>& fmt, Args&&... args)
{
    ImGui::BeginTooltip();
    Gui::text(fmt, std::forward<Args>(args)...);
    ImGui::EndTooltip();
}

template<std::invocable Callback>
void button(const std::string& label, Callback&& callback)
{
    if (ImGui::Button(label.c_str(), ImVec2(0, 0))) { std::invoke(std::forward<Callback>(callback)); }
}

template<std::invocable Callback>
void button(const std::string& label, const ImVec2& size, Callback&& callback)
{
    if (ImGui::Button(label.c_str(), size)) { std::invoke(std::forward<Callback>(callback)); }
}

template<std::invocable Callback>
void image_button(const Texture& texture, const ImVec2& size, const std::string& fallback, Callback&& callback)
{
    if (!texture.loaded()) {
        if (ImGui::Button(fallback.c_str())) { std::invoke(std::forward<Callback>(callback)); }
        return;
    }

    if (ImGui::ImageButton(texture.as_imgui_texture(), size)) { std::invoke(std::forward<Callback>(callback)); }

    constexpr static auto HOVER_THRESHOLD = 0.5F;
    if (ImGui::IsItemHovered() && ImGui::GetCurrentContext()->HoveredIdTimer >= HOVER_THRESHOLD) {
        Gui::tooltip("{}", fallback);
    }
}

void center_content_horizontally(const float content_width);

enum class TableFlags : std::uint32_t
{
    RowBackground          = 1 << 6,
    BordersInnerHorizontal = 1 << 7,
    BordersOuterHorizontal = 1 << 8,
    BordersInnerVertical   = 1 << 9,
    BordersOuterVertical   = 1 << 10,
    BordersInner           = BordersInnerVertical | BordersInnerHorizontal,
    BordersOuter           = BordersOuterVertical | BordersOuterHorizontal,
    Borders                = BordersInner | BordersOuter,
};

[[nodiscard]] constexpr static auto operator|(TableFlags lhs, TableFlags rhs) -> TableFlags
{
    return static_cast<TableFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

template<std::invocable Callback>
void draw_table(
  const std::string&                 name,
  const std::span<const char* const> headers,
  TableFlags                         flags,
  Callback&&                         callback
)
{
    if (ImGui::BeginTable(
          name.c_str(), static_cast<int>(headers.size()), static_cast<int>(std::to_underlying(flags))
        )) {
        for (const auto& header : headers) { ImGui::TableSetupColumn(header); }
        ImGui::TableHeadersRow();
        std::invoke(std::forward<Callback>(callback));
        ImGui::EndTable();
    }
}

template<typename... Callbacks>
void draw_table_row(Callbacks&&... callbacks)
{
    ImGui::TableNextRow();

    int column = 0;
    ((ImGui::TableSetColumnIndex(column++), std::invoke(std::forward<Callbacks>(callbacks))), ...);
}

enum class TreeNodeFlags : std::uint8_t
{
    DefaultOpen = 1 << 5,
};

[[nodiscard]] constexpr static auto operator|(TreeNodeFlags lhs, TreeNodeFlags rhs) -> TreeNodeFlags
{
    return static_cast<TreeNodeFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

template<std::invocable Callback>
void collapsing(const std::string& name, TreeNodeFlags flags, Callback&& callback)
{
    if (ImGui::CollapsingHeader(name.c_str(), std::to_underlying(flags))) {
        ImGui::Indent();
        std::invoke(std::forward<Callback>(callback));
        ImGui::Unindent();
    }
}

[[nodiscard]] auto grid_layout_calc_size(
  const std::size_t rows,
  const std::size_t cols,
  const ImVec2&     available_space = ImGui::GetContentRegionAvail()
) -> ImVec2;

namespace impl
{
template<std::invocable Callback>
void disabled_impl(const bool control, Callback&& callback)
{
    ImGui::BeginDisabled(control);
    std::invoke(std::forward<Callback>(callback));
    ImGui::EndDisabled();
}
} // namespace impl

template<std::invocable Callback>
void disabled_if(const bool control, Callback&& callback)
{
    impl::disabled_impl(control, std::forward<Callback>(callback));
}

template<std::invocable Callback>
void enabled_if(const bool control, Callback&& callback)
{
    impl::disabled_impl(!control, std::forward<Callback>(callback));
}

enum class ToastLevel : std::uint8_t
{
    Info = 0,
    Warning,
    Error,
};

enum class ToastPosition : std::uint8_t
{
    TopLeft = 0,
    TopRight,
    BottomLeft,
    BottomRight,
};

struct [[nodiscard]] Toast final
{
    std::string                  message;
    std::chrono::duration<float> duration;
    ToastLevel                   level;
    ToastPosition                position;
};

class [[nodiscard]] ToastManager final
{
  public:
    static void add(const Toast& toast) { toasts.push_back(toast); }

    static void render()
    {
        const auto delta_time = std::chrono::duration<float>(ImGui::GetIO().DeltaTime);
        const auto spacing    = ImGui::GetStyle().ItemSpacing;

        float y_offset = 0.0F;
        for (auto it = toasts.begin(); it != toasts.end();) {
            auto& toast = *it;

            const auto toast_size = ImVec2(ImGui::CalcTextSize(toast.message.c_str()).x + (spacing.x * 2), 30);
            auto       position   = toast_position_to_vector(toast.position, toast_size);

            toast.duration -= delta_time;
            if (toast.duration <= std::chrono::seconds(0)) {
                it = toasts.erase(it);
                continue;
            }

            if (toast.position == Gui::ToastPosition::BottomLeft || toast.position == Gui::ToastPosition::BottomRight) {
                position.y -= y_offset;
            } else {
                position.y += y_offset;
            }

            constexpr static auto window_flags = Gui::WindowFlags::NoDecoration | Gui::WindowFlags::NoSavedSettings;

            ImGui::SetNextWindowPos(position);
            ImGui::SetNextWindowSize(toast_size);
            Gui::window("##Toast", window_flags, [&] {
                ImGui::PushStyleColor(ImGuiCol_Text, toast_level_to_color(toast.level));
                Gui::text("{}", toast.message);
                ImGui::PopStyleColor();
            });

            y_offset += toast_size.y + spacing.y;
            ++it;
        }
    }

    [[nodiscard]] static auto toast_level_to_color(ToastLevel level) -> ImVec4
    {
        switch (level) {
            case ToastLevel::Info: {
                return { 0.2F, 0.6F, 1.0F, 1.0F };
            }
            case ToastLevel::Warning: {
                return { 1.0F, 0.6F, 0.0F, 1.0F };
            }
            case ToastLevel::Error: {
                return { 1.0F, 0.2F, 0.2F, 1.0F };
            }
        }

        assert(false && "unreachable");
        return { 1.0F, 0.2F, 0.2F, 1.0F };
    }

    [[nodiscard]] static auto toast_position_to_vector(ToastPosition position, const ImVec2& toast_size) -> ImVec2
    {
        const auto spacing       = ImGui::GetStyle().ItemSpacing;
        const auto work_position = ImGui::GetMainViewport()->WorkPos;
        const auto work_size     = ImGui::GetMainViewport()->WorkSize;

        switch (position) {
            case ToastPosition::TopLeft: {
                return {
                    work_position.x + spacing.x,
                    work_position.y + spacing.y,
                };
            }
            case ToastPosition::TopRight: {
                return {
                    work_position.x + work_size.x - toast_size.x - spacing.x,
                    work_position.y + spacing.y,
                };
            }
            case ToastPosition::BottomLeft: {
                return {
                    work_position.x + spacing.x,
                    work_position.y + work_size.y - toast_size.y - spacing.y,
                };
            }
            case ToastPosition::BottomRight: {
                return {
                    work_position.x + work_size.x - toast_size.x - spacing.x,
                    work_position.y + work_size.y - toast_size.y - spacing.y,
                };
            }
        }

        assert(false && "unreachable");
        return { 0, 0 };
    }

  private:
    inline static std::vector<Toast> toasts;
};

void toast(
  const std::string&                 message,
  ToastPosition                      position,
  const std::chrono::duration<float> duration,
  ToastLevel                         level
);

[[nodiscard]] static auto rows_cols_by_count(const std::integral auto count) -> std::pair<std::size_t, std::size_t>
{
    const auto cols = static_cast<int>(std::ceil(std::sqrt(count)));
    const auto rows = static_cast<int>(std::ceil(static_cast<double>(count) / static_cast<double>(cols)));
    return { rows, cols };
}

using IndexGridCallback = std::function<void(const ImVec2&)>;

template<std::invocable<ImVec2, std::size_t> Callback>
void grid(
  const std::integral auto rows,
  const std::integral auto cols,
  const std::integral auto count,
  const ImVec2&            size,
  Callback&&               callback
)
{
    Gui::group([&] {
        std::size_t idx = 0;
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t col = 0; col < cols; ++col) {
                if (idx >= count) { break; }

                const auto remaining_size = Gui::grid_layout_calc_size(rows, cols, size);
                std::invoke(std::forward<Callback>(callback), remaining_size, idx);

                if (col < cols - 1 && idx + 1 < count) { ImGui::SameLine(); }
                ++idx;
            }
        }
    });
}

template<std::invocable<ImVec2, std::size_t> Callback>
void grid(const std::integral auto count, const ImVec2& size, Callback&& callback)
{
    const auto [rows, cols] = rows_cols_by_count(count);
    Gui::grid(rows, cols, count, size, std::forward<Callback>(callback));
}

[[nodiscard]] auto input_text_popup(const std::string& label, bool& condition) -> std::optional<std::string>;

class [[nodiscard]] ComboManager final
{
  public:
    template<typename Enum>
    static void register_combo_by_id(const ImGuiID id, const std::uint8_t default_index = 0)
      requires(std::is_scoped_enum_v<Enum>)
    {
        auto key = std::make_pair(std::type_index(typeid(Enum)), id);
        if (!states.contains(key)) { states[key] = default_index; }
    }

    template<typename Enum>
    [[nodiscard]] static auto selected_variant_by_id(const ImGuiID id)
      -> std::uint8_t& requires(std::is_scoped_enum_v<Enum>)
    {
        auto key = std::make_pair(std::type_index(typeid(Enum)), id);
        return std::any_cast<std::uint8_t&>(states[key]);
    }

  private:
    using Key = std::pair<std::type_index, ImGuiID>;
    struct KeyHash
    {
        [[nodiscard]] auto operator()(const Key& key) const -> std::size_t
        {
            return std::hash<std::type_index> {}(key.first) ^ std::hash<ImGuiID> {}(key.second);
        }
    };

    inline static std::unordered_map<Key, std::any, KeyHash> states;
};

template<typename Enum, std::invocable<Enum> Callback>
void combo(const std::string& name, const std::span<const Enum> values, Enum currently_active, Callback&& callback)
  requires(std::is_scoped_enum_v<Enum>)
{
    const auto id = ImGui::GetID(name.c_str());
    ComboManager::register_combo_by_id<Enum>(id, std::to_underlying(currently_active));
    auto& selected_idx = ComboManager::selected_variant_by_id<Enum>(id);

    float max_value_length = 0;
    for (std::size_t idx = 0; idx < values.size(); ++idx) {
        const auto variant = values[idx];
        const auto name    = std::format("{}", variant);
        max_value_length   = std::max(max_value_length, ImGui::CalcTextSize(name.c_str()).x);
    }

    const auto total_width = max_value_length + ImGui::GetFrameHeight() + (ImGui::GetStyle().FramePadding.x * 2.0F);

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - total_width);
    ImGui::SetNextItemWidth(total_width);
    if (ImGui::BeginCombo("##Schedule Policy", std::format("{}", values[selected_idx]).c_str())) {
        for (std::size_t idx = 0; idx < values.size(); ++idx) {
            const auto variant     = values[idx];
            bool       is_selected = (selected_idx == idx);

            if (ImGui::Selectable(std::format("{}", variant).c_str(), is_selected)) {
                selected_idx = static_cast<std::uint8_t>(idx);
                std::invoke(std::forward<Callback>(callback), variant);
            }

            if (is_selected) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
    }
}

namespace Plotting
{

class [[nodiscard]] RingBuffer final
{
  public:
    explicit RingBuffer(int capacity = 2000)
      : capacity { capacity }
    {
        data.reserve(capacity);
    }

    void emplace_point(const float x, const float y)
    {
        if (data.size() < capacity) {
            data.push_back(ImVec2(x, y));
            return;
        }

        data[cursor] = ImVec2(x, y);
        cursor       = (cursor + 1) % capacity;
    }

    void clear() { data.clear(); }

    [[nodiscard]] auto operator[](const int index) const -> const ImVec2& { return data[index]; }

    [[nodiscard]] auto size() const -> int { return data.size(); }

    [[nodiscard]] auto offset() const -> int { return cursor; }

  private:
    int              capacity;
    int              cursor = 0;
    ImVector<ImVec2> data;
};

enum class AxisFlags : std::uint16_t
{
    None         = 0,
    NoTickMarks  = 1 << 2,
    NoTickLabels = 1 << 3,
    AutoFit      = 1 << 11,
};

[[nodiscard]] constexpr static auto operator|(AxisFlags lhs, AxisFlags rhs) -> AxisFlags
{
    return static_cast<AxisFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

struct [[nodiscard]] PlotOpts final
{
    AxisFlags x_axis_flags = AxisFlags::None;
    AxisFlags y_axis_flags = AxisFlags::None;

    std::optional<double> x_min = std::nullopt;
    std::optional<double> x_max = std::nullopt;
    std::optional<double> y_min = std::nullopt;
    std::optional<double> y_max = std::nullopt;

    std::optional<std::string> x_label     = std::nullopt;
    std::optional<std::string> y_label     = std::nullopt;
    std::optional<ImVec4>      color       = std::nullopt;
    std::optional<float>       line_weight = std::nullopt;

    bool scrollable  = true;
    bool maximizable = true;
};

template<std::invocable Callback>
void plot(const std::string& title, const ImVec2& size, const PlotOpts& opts, Callback&& callback)
{
    static std::unordered_map<std::string, bool> maximized_map;

    const bool was_maximized = maximized_map[title];
    const auto plot_size     = was_maximized ? ImGui::GetIO().DisplaySize : size;

    if (opts.maximizable && maximized_map[title]) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);

        ImGui::Begin(
          "MaximizedPlotWindow",
          nullptr,
          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoCollapse
        );

        ImGui::PopStyleVar(2);
    }

    if (ImPlot::BeginPlot(title.c_str(), plot_size)) {
        ImPlot::SetupAxes(
          opts.x_label.has_value() ? opts.x_label->c_str() : nullptr,
          opts.y_label.has_value() ? opts.y_label->c_str() : nullptr,
          std::to_underlying(opts.x_axis_flags),
          std::to_underlying(opts.y_axis_flags)
        );

        const auto default_range = ImPlotRange(0, 1);

        ImPlot::SetupAxisLimits(
          ImAxis_X1,
          opts.x_min.value_or(default_range.Min),
          opts.x_max.value_or(default_range.Max),
          opts.scrollable ? ImGuiCond_Once : ImGuiCond_Always
        );

        ImPlot::SetupAxisLimits(
          ImAxis_Y1,
          opts.y_min.value_or(default_range.Min),
          opts.y_max.value_or(default_range.Max),
          opts.scrollable ? ImGuiCond_Once : ImGuiCond_Always
        );

        if (opts.color) { ImPlot::PushStyleColor(ImPlotCol_Line, opts.color.value()); }
        if (opts.line_weight) { ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, opts.line_weight.value()); }

        std::invoke(std::forward<Callback>(callback));

        if (opts.maximizable && ImPlot::IsPlotHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            maximized_map[title] = !maximized_map[title];
            ImPlot::EndPlot();
            if (was_maximized) { ImGui::End(); }
            return;
        }

        if (opts.line_weight) { ImPlot::PopStyleVar(); }
        if (opts.color) { ImPlot::PopStyleColor(); }

        ImPlot::EndPlot();
    }

    if (opts.maximizable && was_maximized) { ImGui::End(); }
}

enum class LineFlags : std::uint8_t
{
    None = 0,
};

[[nodiscard]] constexpr static auto operator|(LineFlags lhs, LineFlags rhs) -> LineFlags
{
    return static_cast<LineFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

void line(const std::string& label, const RingBuffer& buffer, LineFlags flags);

enum class SubplotFlags : std::uint8_t
{
    None = 0,
};

[[nodiscard]] constexpr static auto operator|(SubplotFlags lhs, SubplotFlags rhs) -> SubplotFlags
{
    return static_cast<SubplotFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

template<std::invocable<ImVec2> Callback>
void subplots(
  const std::string&       title,
  const std::integral auto count,
  const ImVec2&            size,
  SubplotFlags             flags,
  Callback&&               callback
)
{
    const auto [rows, cols] = rows_cols_by_count(count);

    ImPlot::BeginSubplots(
      title.c_str(), static_cast<int>(rows), static_cast<int>(cols), size, std::to_underlying(flags)
    );
    Gui::grid(rows, cols, count, size, std::forward<Callback>(callback));
    ImPlot::EndSubplots();
}

void bars(const std::span<const std::string> labels, const std::span<const double> values);

} // namespace Plotting

void draw_call(GLFWwindow* window, const ImVec4& clear_color);

} // namespace Gui
