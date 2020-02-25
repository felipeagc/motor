#include <motor/engine/imgui_impl.h>

#include <motor/base/log.h>
#include <motor/base/math.h>
#include <motor/base/allocator.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/engine.h>
#include <motor/engine/assets/pipeline_asset.h>

struct MtImguiContext
{
    MtEngine *engine;
    MtPipelineAsset *pipeline;
    MtImage *atlas;
    MtSampler *sampler;
    bool mouse_just_pressed[5];
};

MtImguiContext *mt_imgui_create(MtEngine *engine)
{
    MtImguiContext *ctx = mt_alloc(engine->alloc, sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));

    ctx->engine = engine;
    ctx->pipeline = engine->imgui_pipeline;

    igCreateContext(NULL);
    igStyleColorsDark(NULL);

    ImGuiStyle *style = igGetStyle();
    style->WindowRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->TabRounding = 0.0f;
    style->WindowBorderSize = 0.0f;
    style->TabBorderSize = 0.0f;
    style->FrameBorderSize = 0.0f;

    ImVec4 text_color = {0.8f, 0.8f, 0.8f, 1.00f};
    ImVec4 background = {0.079f, 0.084f, 0.098f, 0.976f};
    ImVec4 background_darker = {0.04f, 0.04f, 0.04f, 1.00f};
    ImVec4 background_brighter = {0.12f, 0.13f, 0.16f, 1.00f};
    ImVec4 border_color = {0.20f, 0.22f, 0.26f, 1.00f};

    ImVec4 main_color = {0.168f, 0.189f, 0.26f, 1.00f};
    ImVec4 main_color_hovered = {0.332f, 0.406f, 0.647f, 1.00f};
    ImVec4 main_color_active = {0.213f, 0.260f, 0.414f, 1.0f};

    ImVec4 *colors = style->Colors;
    colors[ImGuiCol_Text] = text_color;
    colors[ImGuiCol_WindowBg] = background;
    colors[ImGuiCol_Border] = border_color;
    colors[ImGuiCol_FrameBg] = background_darker;
    colors[ImGuiCol_FrameBgHovered] = background_brighter;
    colors[ImGuiCol_FrameBgActive] = main_color;
    colors[ImGuiCol_TitleBg] = background_darker;
    colors[ImGuiCol_TitleBgActive] = main_color_active;
    colors[ImGuiCol_TitleBgCollapsed] = background_darker;
    colors[ImGuiCol_Button] = main_color;
    colors[ImGuiCol_ButtonHovered] = main_color_hovered;
    colors[ImGuiCol_ButtonActive] = main_color_active;
    colors[ImGuiCol_Header] = main_color;
    colors[ImGuiCol_HeaderHovered] = main_color_hovered;
    colors[ImGuiCol_HeaderActive] = main_color_active;
    colors[ImGuiCol_ResizeGrip] = main_color;
    colors[ImGuiCol_ResizeGripHovered] = main_color_hovered;
    colors[ImGuiCol_ResizeGripActive] = main_color_active;
    colors[ImGuiCol_CheckMark] = main_color;
    colors[ImGuiCol_SliderGrab] = main_color;
    colors[ImGuiCol_SliderGrabActive] = background;
    colors[ImGuiCol_Tab] = main_color;
    colors[ImGuiCol_TabHovered] = main_color_hovered;
    colors[ImGuiCol_TabActive] = main_color_active;
    colors[ImGuiCol_TabUnfocused] = background_brighter;
    colors[ImGuiCol_TabUnfocusedActive] = background_darker;
    colors[ImGuiCol_PlotLines] = main_color;
    colors[ImGuiCol_PlotLinesHovered] = main_color_hovered;
    colors[ImGuiCol_PlotHistogram] = main_color;
    colors[ImGuiCol_PlotHistogramHovered] = main_color_hovered;
    colors[ImGuiCol_TextSelectedBg] = main_color_active;

    ImGuiIO *io = igGetIO();

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io->KeyMap[ImGuiKey_Tab] = MT_KEY_TAB;
    io->KeyMap[ImGuiKey_LeftArrow] = MT_KEY_LEFT;
    io->KeyMap[ImGuiKey_RightArrow] = MT_KEY_RIGHT;
    io->KeyMap[ImGuiKey_UpArrow] = MT_KEY_UP;
    io->KeyMap[ImGuiKey_DownArrow] = MT_KEY_DOWN;
    io->KeyMap[ImGuiKey_PageUp] = MT_KEY_PAGE_UP;
    io->KeyMap[ImGuiKey_PageDown] = MT_KEY_PAGE_DOWN;
    io->KeyMap[ImGuiKey_Home] = MT_KEY_HOME;
    io->KeyMap[ImGuiKey_End] = MT_KEY_END;
    io->KeyMap[ImGuiKey_Insert] = MT_KEY_INSERT;
    io->KeyMap[ImGuiKey_Delete] = MT_KEY_DELETE;
    io->KeyMap[ImGuiKey_Backspace] = MT_KEY_BACKSPACE;
    io->KeyMap[ImGuiKey_Space] = MT_KEY_SPACE;
    io->KeyMap[ImGuiKey_Enter] = MT_KEY_ENTER;
    io->KeyMap[ImGuiKey_Escape] = MT_KEY_ESCAPE;
    io->KeyMap[ImGuiKey_KeyPadEnter] = MT_KEY_KP_ENTER;
    io->KeyMap[ImGuiKey_A] = MT_KEY_A;
    io->KeyMap[ImGuiKey_C] = MT_KEY_C;
    io->KeyMap[ImGuiKey_V] = MT_KEY_V;
    io->KeyMap[ImGuiKey_X] = MT_KEY_X;
    io->KeyMap[ImGuiKey_Y] = MT_KEY_Y;
    io->KeyMap[ImGuiKey_Z] = MT_KEY_Z;

    ctx->sampler = mt_render.create_sampler(
        ctx->engine->device,
        &(MtSamplerCreateInfo){.mag_filter = MT_FILTER_LINEAR, .min_filter = MT_FILTER_LINEAR});

    unsigned char *pixels;
    int width, height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, NULL);
    size_t upload_size = width * height * 4 * sizeof(char);

    ctx->atlas = mt_render.create_image(
        ctx->engine->device,
        &(MtImageCreateInfo){.format = MT_FORMAT_RGBA8_UNORM, .width = width, .height = height});

    mt_render.transfer_to_image(
        ctx->engine->device, &(MtImageCopyView){.image = ctx->atlas}, upload_size, pixels);

    io->Fonts->TexID = (void *)ctx->atlas;

    return ctx;
}

void mt_imgui_handle_event(MtImguiContext *ctx, MtEvent *event)
{
    ImGuiIO *io = igGetIO();

    switch (event->type)
    {
        case MT_EVENT_SCROLLED: {
            io->MouseWheelH += (float)event->scroll.x;
            io->MouseWheel += (float)event->scroll.y;
            break;
        }
        case MT_EVENT_CODEPOINT_INPUT: {
            ImGuiIO_AddInputCharacter(io, event->codepoint);
            break;
        }
        case MT_EVENT_BUTTON_PRESSED: {
            if (event->mouse.button >= 0 &&
                event->mouse.button < MT_LENGTH(ctx->mouse_just_pressed))
            {
                ctx->mouse_just_pressed[event->mouse.button] = true;
            }
            break;
        }
        case MT_EVENT_KEY_PRESSED:
        case MT_EVENT_KEY_RELEASED: {
            io->KeysDown[event->keyboard.key] = (event->type == MT_EVENT_KEY_PRESSED);

            io->KeyCtrl = io->KeysDown[MT_KEY_LEFT_CONTROL] || io->KeysDown[MT_KEY_RIGHT_CONTROL];
            io->KeyShift = io->KeysDown[MT_KEY_LEFT_SHIFT] || io->KeysDown[MT_KEY_RIGHT_SHIFT];
            io->KeyAlt = io->KeysDown[MT_KEY_LEFT_ALT] || io->KeysDown[MT_KEY_RIGHT_ALT];
            break;
        }
        default: break;
    }
}

void mt_imgui_begin(MtImguiContext *ctx)
{
    ImGuiIO *io = igGetIO();

    for (uint32_t i = 0; i < MT_LENGTH(io->MouseDown); i++)
    {
        io->MouseDown[i] =
            ctx->mouse_just_pressed[i] ||
            mt_window.get_mouse_button(ctx->engine->window, i) != MT_INPUT_STATE_RELEASE;
        ctx->mouse_just_pressed[i] = false;
    }

    const ImVec2 mouse_pos_backup = io->MousePos;
    io->MousePos = (ImVec2){-FLT_MAX, -FLT_MAX};

    if (io->WantSetMousePos)
    {
        mt_window.set_cursor_pos(
            ctx->engine->window, (int32_t)mouse_pos_backup.x, (int32_t)mouse_pos_backup.y);
    }
    else
    {
        int32_t mouse_x, mouse_y;
        mt_window.get_cursor_pos(ctx->engine->window, &mouse_x, &mouse_y);
        io->MousePos = (ImVec2){(float)mouse_x, (float)mouse_y};
    }

    // Setup display size (every frame to accommodate for window resizing)
    uint32_t w, h;
    mt_window.get_size(ctx->engine->window, &w, &h);
    io->DisplaySize = (ImVec2){.x = (float)w, .y = (float)h};
    io->DisplayFramebufferScale = (ImVec2){.x = 1.0f, .y = 1.0f};

    // Setup time step
    io->DeltaTime = mt_render.swapchain_get_delta_time(ctx->engine->swapchain);
}

void mt_imgui_render(MtImguiContext *ctx, MtCmdBuffer *cb)
{
    ImDrawData *draw_data = igGetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates !=
    // framebuffer coordinates)
    int32_t fb_width = (int32_t)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int32_t fb_height = (int32_t)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0) return;

    // Create or resize the vertex/index buffers
    size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

    ImDrawVert *vertices = mt_render.cmd_bind_vertex_data(cb, vertex_size);
    ImDrawIdx *indices = mt_render.cmd_bind_index_data(cb, index_size, MT_INDEX_TYPE_UINT16);

    // Upload vertex/index data into a single contiguous GPU buffer
    {
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList *cmd_list = draw_data->CmdLists[n];
            memcpy(
                vertices, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(indices, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vertices += cmd_list->VtxBuffer.Size;
            indices += cmd_list->IdxBuffer.Size;
        }
    }

    mt_render.cmd_bind_pipeline(cb, ctx->pipeline->pipeline);

    struct
    {
        float scale[2];
        float translate[2];
    } uniform;

    uniform.scale[0] = 2.0f / draw_data->DisplaySize.x;
    uniform.scale[1] = 2.0f / draw_data->DisplaySize.y;
    uniform.translate[0] = -1.0f - draw_data->DisplayPos.x * uniform.scale[0];
    uniform.translate[1] = -1.0f - draw_data->DisplayPos.y * uniform.scale[1];

    mt_render.cmd_bind_uniform(cb, &uniform, sizeof(uniform), 0, 0);

    mt_render.cmd_set_viewport(
        cb,
        &(MtViewport){.x = 0,
                      .y = 0,
                      .width = (float)fb_width,
                      .height = (float)fb_height,
                      .min_depth = 0.0f,
                      .max_depth = 1.0f});

    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;

    uint32_t global_vtx_offset = 0;
    uint32_t global_idx_offset = 0;
    for (uint32_t n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        for (uint32_t cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer.Data[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                mt_render.cmd_bind_sampler(cb, ctx->sampler, 0, 1);
                mt_render.cmd_bind_image(cb, (MtImage *)pcmd->TextureId, 0, 2);

                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f &&
                    clip_rect.w >= 0.0f)
                {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clip_rect.x < 0.0f) clip_rect.x = 0.0f;
                    if (clip_rect.y < 0.0f) clip_rect.y = 0.0f;

                    mt_render.cmd_set_scissor(
                        cb,
                        (int32_t)(clip_rect.x),
                        (int32_t)(clip_rect.y),
                        (uint32_t)(clip_rect.z - clip_rect.x),
                        (uint32_t)(clip_rect.w - clip_rect.y));

                    mt_render.cmd_draw_indexed(
                        cb,
                        (uint32_t)pcmd->ElemCount,
                        1,
                        pcmd->IdxOffset + global_idx_offset,
                        pcmd->VtxOffset + global_vtx_offset,
                        0);
                }
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

void mt_imgui_destroy(MtImguiContext *ctx)
{
    mt_render.destroy_sampler(ctx->engine->device, ctx->sampler);
    mt_render.destroy_image(ctx->engine->device, ctx->atlas);
    mt_free(ctx->engine->alloc, ctx);
}
