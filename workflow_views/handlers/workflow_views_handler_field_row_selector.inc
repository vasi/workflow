<?php
/**
 * @file
 * Views field handler. Outputs a checkbox for selecting a row.
 * Implements the Views Form API.
 *
 * This is copied from https://www.drupal.org/project/issues/views_row_selector,
 * which is a copy of https://www.drupal.org/project/views_send.
 */

class workflow_views_handler_field_row_selector extends views_handler_field {
  /**
   * If the view is using a table style, provide a
   * placeholder for a "select all" checkbox.
   */
  function label() {
    if ($this->view->style_plugin instanceof views_plugin_style_table) {
      return '<!--views-row-selector-select-all-->';
    }
    else {
      return parent::label();
    }
  }

  function query() {
    // Do nothing.
  }

  function render($values) {
    return '<!--form-item-' . $this->options['id'] . '--' . $this->view->row_index . '-->';
  }

  /**
   * The form which replaces the placeholder from render().
   */
  function views_form(&$form, &$form_state) {
    // The view is empty, abort.
    if (empty($this->view->result)) {
      return;
    }

    $form[$this->options['id']] = array(
      '#tree' => TRUE,
    );
    foreach ($this->view->result as $row_index => $row) {
      $form[$this->options['id']][$row_index] = array(
        '#type' => 'checkbox',
        '#return_value' => $row->nid,
        //'#default_value' => FALSE,
        '#attributes' => array('class' => array('views-row-selector-select')),
      );
    }
  }

  function views_form_validate($form, &$form_state) {
    $field_name = $this->options['id'];
    $selection = array_filter($form_state['values'][$field_name]);

    if (empty($selection)) {
      form_set_error($field_name, t('Please select at least one item.'));
    }
  }
}