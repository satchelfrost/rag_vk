/* rag_vk - v1.0.0 - MIT license - https://github.com/satchelfrost/rag_vk

   **WARNING** Probably don't use this, especially not v0.0.0

   This file is an attempt to have a vulkan stb-style header-only library.
   Currently, the header-only goal isn't being met very well. However, this
   file is still being used in multiple projects, and the versions are starting to diverge.
   Therefore, I've collected it into a repo to have a single source of authority,
   and track different versions.

   TODO: usage exaplanation and conventions

*/

/*
 * Goals for v1.0.0
 * 1) don't add deprecated functions
 * 2) Fat_Vk*CreateInfo macro in use
 * 3) thin vulkan functions should have no dependencies on global context
 * 4) For thin vulkan functions if certain parameters are needed and not passed in, then
 *    a default one may be used. For example, vkCmdBeginRenderPass requires a command buffer
 *    so for the thin vulkan wrapper vk_cmd_begin_render_pass, if a command buffer is not
 *    passed in, then we should assume a default.
 *
 * */
