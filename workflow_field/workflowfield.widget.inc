<?php

/**
 * @file
 * Defines a Workflow field, widget (sfrom options.module) and formatter (from list field).
 * All hooks are wrapper functions for a D8-style WorkflowDefaultWidget object.
 */

/**
 * Implements hook_field_widget_info().
 */
function workflowfield_field_widget_info() {
  return WorkflowDefaultWidget::settings();
}

/**
 * Implements hook_field_widget_settings_form().
 */
function workflowfield_field_widget_settings_form($field, $instance) {
  $form = array();
  $form_state = array();

  // The form element is created by a D8-like object.
  $widget = new WorkflowDefaultWidget($field, $instance);
  return $widget->settingsForm($form, $form_state, $has_data = 0);
}

/**
 * Implements hook_field_widget_form().
 */
function workflowfield_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  // This is a wrapper function for the 'workflow form' Widget. $form is modified by reference.
  $widget = new WorkflowDefaultWidget($field, $instance);
  $element += $widget->formElement($items, $delta, $element, $langcode, $form, $form_state);

  return $element;
}

/**
 * Form element validation handler for workflow element.
 */
function workflowfield_field_widget_validate($element, &$form_state) {
//  if ($element['#required'] && $element['#value'] == '_none') {
//    form_error($element, t('!name field is required.', array('!name' => $element['#title'])));
//  }
//  // Transpose selections from field => delta to delta => field, turning
//  // multiple selected options into multiple parent elements.
//  $items = _workflowfield_form_to_storage($element);
//  form_set_value($element, $items, $form_state);
}
