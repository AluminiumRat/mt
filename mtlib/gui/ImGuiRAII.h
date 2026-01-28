#pragma once

#include <ImGui.h>

namespace mt
{
  //  Создание окна ImGui
  //  RAII обертка вокруг ImGui::Begin и ImGui::End
  class ImGuiWindow
  {
  public:
    inline ImGuiWindow( const char* name,
                        bool* p_open = NULL,
                        ImGuiWindowFlags flags = 0) noexcept;
    ImGuiWindow(const ImGuiWindow&) = delete;
    ImGuiWindow& operator = (const ImGuiWindow&) = delete;
    inline ~ImGuiWindow() noexcept;

    //  Вызов ImGui::Begin вернул true, то есть окно не свернуто и не перекрыто
    //  другим окном
    inline bool visible() const noexcept;

    //  Закончить описание окна до вызова деструктора
    inline void end() noexcept;

  private:
    bool _visible;
    bool _finished;
  };

  //  Главное меню
  //  RAII обертка вокруг ImGui::BeginMainMenuBar и ImGui::EndMainMenuBar
  class ImGuiMainMenuBar
  {
  public:
    inline ImGuiMainMenuBar() noexcept;
    ImGuiMainMenuBar(const ImGuiMainMenuBar&) = delete;
    ImGuiMainMenuBar& operator = (const ImGuiMainMenuBar&) = delete;
    inline ~ImGuiMainMenuBar() noexcept;

    // ImGui::BeginMainMenuBar вернул true, можно заполнять меню
    inline bool created() const noexcept;

    //  Закончить заполнение меню до вызова деструктора
    inline void end() noexcept;

  private:
    bool _created;
  };

  //  RAII обертка вокруг ImGui::BeginMenu и ImGui::EndMenu
  class ImGuiMenu
  {
  public:
    inline ImGuiMenu(const char* label, bool enabled = true) noexcept;
    ImGuiMenu(const ImGuiMenu&) = delete;
    ImGuiMenu& operator = (const ImGuiMenu&) = delete;
    inline ~ImGuiMenu() noexcept;

    // ImGui::BeginMenu вернул true, можно заполнять меню
    inline bool created() const noexcept;

    //  Закончить заполнение меню до вызова деструктора
    inline void end() noexcept;

  private:
    bool _created;
  };

  //  RAII обертка вокруг ImGui::BeginCombo и ImGui::EndCombo
  class ImGuiCombo
  {
  public:
    inline ImGuiCombo(const char* label,
                      const char* preview_value,
                      ImGuiComboFlags flags = 0) noexcept;
    ImGuiCombo(const ImGuiCombo&) = delete;
    ImGuiCombo& operator = (const ImGuiCombo&) = delete;
    inline ~ImGuiCombo() noexcept;

    // ImGui::BeginCombo вернул true, можно заполнять содержимое
    inline bool created() const noexcept;

    //  Закончить заполнение до вызова деструктора
    inline void end() noexcept;

  private:
    bool _created;
  };

  //  RAII обертка вокруг ImGui::BeginPopupContextItem и ImGui::EndPopup
  class ImGuiPopupContextItem
  {
  public:
    inline ImGuiPopupContextItem( const char* str_id = NULL,
                                  ImGuiPopupFlags popup_flags = 1) noexcept;
    ImGuiPopupContextItem(const ImGuiPopupContextItem&) = delete;
    ImGuiPopupContextItem& operator = (const ImGuiPopupContextItem&) = delete;
    inline ~ImGuiPopupContextItem() noexcept;

    // ImGui::BeginPopupContextItem вернул true, можно заполнять попап
    inline bool created() const noexcept;

    //  Закончить заполнение до вызова деструктора
    inline void end() noexcept;

  private:
    bool _created;
  };

  //  RAII обертка вокруг ImGui::PushID и ImGui::PopID
  class ImGuiPushID
  {
  public:
    inline ImGuiPushID(const char* str_id) noexcept;
    inline ImGuiPushID( const char* str_id_begin,
                        const char* str_id_end) noexcept;
    inline ImGuiPushID(const void* ptr_id) noexcept;
    inline ImGuiPushID(int int_id) noexcept;
    ImGuiPushID(const ImGuiPushID&) = delete;
    ImGuiPushID& operator = (const ImGuiPushID&) = delete;
    inline ~ImGuiPushID() noexcept;

    //  Закончить область действия до вызова деструктора
    inline void pop() noexcept;

  private:
    bool _active;
  };

  //  RAII обертка вокруг ImGui::PushStyleColor и ImGui::PopStyleColor
  class ImGuiPushStyleColor
  {
  public:
    inline ImGuiPushStyleColor(ImGuiCol idx, ImU32 col) noexcept;
    inline ImGuiPushStyleColor(ImGuiCol idx, const ImVec4& col) noexcept;
    ImGuiPushStyleColor(const ImGuiPushStyleColor&) = delete;
    ImGuiPushStyleColor& operator = (const ImGuiPushStyleColor&) = delete;
    inline ~ImGuiPushStyleColor() noexcept;

    //  Закончить область действия до вызова деструктора
    inline void pop() noexcept;

  private:
    bool _active;
  };

  //  RAII обертка вокруг ImGui::TreeNode и ImGui::TreePop()
  class ImGuiTreeNode
  {
  public:
    inline ImGuiTreeNode(const char* label) noexcept;
    ImGuiTreeNode(const ImGuiTreeNode&) = delete;
    ImGuiTreeNode& operator = (const ImGuiTreeNode&) = delete;
    inline ~ImGuiTreeNode() noexcept;

    //  Открыта или нет нода
    inline bool open() const noexcept;

    //  Закончить область действия до вызова деструктора
    inline void pop() noexcept;

  private:
    bool _active;
  };

  inline ImGuiWindow::ImGuiWindow(const char* name,
                                  bool* p_open,
                                  ImGuiWindowFlags flags) noexcept :
    _finished(false)
  {
    _visible = ImGui::Begin(name, p_open, flags);
  }

  inline ImGuiWindow::~ImGuiWindow() noexcept
  {
    end();
  }

  inline bool ImGuiWindow::visible() const noexcept
  {
    return !_finished && _visible;
  }

  inline void ImGuiWindow::end() noexcept
  {
    if(!_finished)
    {
      ImGui::End();
      _finished = true;
    }
  }

  inline ImGuiMainMenuBar::ImGuiMainMenuBar() noexcept :
    _created(ImGui::BeginMainMenuBar())
  {
  }

  inline ImGuiMainMenuBar::~ImGuiMainMenuBar() noexcept
  {
    end();
  }

  inline bool ImGuiMainMenuBar::created() const noexcept
  {
    return _created;
  }

  inline void ImGuiMainMenuBar::end() noexcept
  {
    if(_created)
    {
      ImGui::EndMainMenuBar();
      _created = false;
    }
  }

  inline ImGuiMenu::ImGuiMenu(const char* label, bool enabled) noexcept :
    _created(ImGui::BeginMenu(label, enabled))
  {
  }

  inline ImGuiMenu::~ImGuiMenu() noexcept
  {
    end();
  }

  inline bool ImGuiMenu::created() const noexcept
  {
    return _created;
  }

  inline void ImGuiMenu::end() noexcept
  {
    if(_created)
    {
      ImGui::EndMenu();
      _created = false;
    }
  }

  inline ImGuiCombo::ImGuiCombo(const char* label,
                                const char* preview_value,
                                ImGuiComboFlags flags) noexcept :
    _created(ImGui::BeginCombo(label, preview_value, flags))
  {
  }

  inline ImGuiCombo::~ImGuiCombo() noexcept
  {
    end();
  }

  inline bool ImGuiCombo::created() const noexcept
  {
    return _created;
  }

  inline void ImGuiCombo::end() noexcept
  {
    if (_created)
    {
      ImGui::EndCombo();
      _created = false;
    }
  }

  inline ImGuiPopupContextItem::ImGuiPopupContextItem(
                                        const char* str_id,
                                        ImGuiPopupFlags popup_flags) noexcept :
    _created(ImGui::BeginPopupContextItem(str_id, popup_flags))
  {
  }

  inline ImGuiPopupContextItem::~ImGuiPopupContextItem() noexcept
  {
    end();
  }

  inline bool ImGuiPopupContextItem::created() const noexcept
  {
    return _created;
  }

  inline void ImGuiPopupContextItem::end() noexcept
  {
    if (_created)
    {
      ImGui::EndPopup();
      _created = false;
    }
  }

  inline ImGuiPushID::ImGuiPushID(const char* str_id) noexcept :
    _active(true)
  {
    ImGui::PushID(str_id);
  }

  inline ImGuiPushID::ImGuiPushID( const char* str_id_begin,
                      const char* str_id_end) noexcept :
    _active(true)
  {
    ImGui::PushID(str_id_begin, str_id_end);
  }

  inline ImGuiPushID::ImGuiPushID(const void* ptr_id) noexcept :
    _active(true)
  {
    ImGui::PushID(ptr_id);
  }

  inline ImGuiPushID::ImGuiPushID(int int_id) noexcept :
    _active(true)
  {
    ImGui::PushID(int_id);
  }

  inline ImGuiPushID::~ImGuiPushID() noexcept
  {
    pop();
  }

  inline void ImGuiPushID::pop() noexcept
  {
    if(_active)
    {
      ImGui::PopID();
      _active = false;
    }
  }

  inline ImGuiPushStyleColor::ImGuiPushStyleColor(ImGuiCol idx,
                                                  ImU32 col) noexcept :
    _active(true)
  {
    ImGui::PushStyleColor(idx, col);
  }

  inline ImGuiPushStyleColor::ImGuiPushStyleColor(ImGuiCol idx,
                                                  const ImVec4& col) noexcept :
    _active(true)
  {
    ImGui::PushStyleColor(idx, col);
  }

  inline ImGuiPushStyleColor::~ImGuiPushStyleColor() noexcept
  {
    pop();
  }

  inline void ImGuiPushStyleColor::pop() noexcept
  {
    if(_active)
    {
      ImGui::PopStyleColor();
      _active = false;
    }
  }

  inline ImGuiTreeNode::ImGuiTreeNode(const char* label) noexcept :
    _active(false)
  {
    _active = ImGui::TreeNode(label);
  }

  inline ImGuiTreeNode::~ImGuiTreeNode() noexcept
  {
    pop();
  }

  inline void ImGuiTreeNode::pop() noexcept
  {
    if(_active)
    {
      ImGui::TreePop();
      _active = false;
    }
  }

  inline bool ImGuiTreeNode::open() const noexcept
  {
    return _active;
  }
}