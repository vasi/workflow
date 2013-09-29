<?php

/**
 * @file
 * Defines a Workflow formatter.
 * You won't find a DefaultFormatter, because:
 * - The 'default' formatter provided by the List module;
 * - The 'workflow' formatter is only representing the WorkflowDefault Widget.
 *
 * All hooks are wrapper functions for a D8-style WorkflowDefaultWidget object.
 */

/**
 * Implements hook_field_formatter_info().
 */
function Workflowfield_field_formatter_info() {
  return WorkflowDefaultWidget::settings();
}

/**
 * Implements hook_field_formatter_view().
 *
 * This is a copy of D7 workflow_node_view($node, $view_mode, $langcode)
 * combined with D7 workflow_tab_form($form, $form_state, $node, $workflow, $states, $current),
 * showing the default Workflow widget.
 */
function workflowfield_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $field_name = $field['field_name'];
  $current_sid = _workflow_get_sid_by_items($items);

  // First compose the current value with the normal formatter from list.module.
  $list_display = $instance['display']['default'];
  $list_display['type'] = 'list_default';
  // Format the $field_name field to a normal $items array.
  $list_entity = $entity;
  $list_entity->{$field_name}['und'][0]['value'] = $current_sid;
  // Render the field.
  $list_element = field_view_field($entity_type, $list_entity, $field_name, $list_display, $langcode);
  
  if ($entity_type == 'comment' || // No Workflow form on a comment display. (Also, this avoids a lot of error messages.)
      $entity->status == 0 ||      // If the node is not published,  
      !user_access('show workflow state form') // IF user has no access to Workflow Form.
      ) {
    return $list_element;
  }

  $state = new WorkflowState($current_sid);
  $workflow = $state->getWorkflow();
  $choices = workflow_field_choices($entity, $force = FALSE, $state); // All valid to-states from current sid.
  $states = $workflow->getOptions(); // All states of the Workflow, in format $key => $label.

  // Only build form if user has possible target state(s).
  if (!workflow_show_form($current_sid, $workflow, $choices)) {
    return $list_element;
  }

  // Add some data for the form.
  $delta = 0;
  $element = array();

  // Add the current value to the form.
  // Make sure the current value is before the form. (which has weight = 0.005)
  $list_element['#weight'] = 0;
  $element['workflow_current_state'] = $list_element;

  // Emulate the D7 arrays. These values will be copied to $form_state upon submit.
  // The first 2 values emulate drupal behaviour.
  $element['#entity_type'] = $entity_type;
  $element['#entity'] = $entity;
  // The following 2 values are added for later reference, too.
  $element['workflow_field'] = array('#type' => 'value', '#value' => $field);
  $element['workflow_instance'] = array('#type' => 'value', '#value' => $instance);

  // Make sure the Submit button is added to the form.
  $instance['widget']['settings']['submit'] = TRUE;

/*
  // This is a wrapper function for the 'workflow form' Widget. $form is modified by reference.
  $widget = new WorkflowDefaultWidget($field, $instance, $entity_type, $entity);
  $form = array();
  $form_state = array();
  $element += $widget->formElement($items, $delta, $element, $langcode, $form, $form_state);
*/

  // Add the form/widget to the formatter, and include the nid in the form id,
  // to allow multiple forms per page (in listings, with hook_forms() ).
  // Ultimately, this is a wrapper for the WorkflowDefaultWidget.
  $element += drupal_get_form(WORKFLOWFIELD_FORM . '_$entity->nid', $field, $instance, $langcode, $items, $delta, $element);

  return $element;
}

function workflowfield_field_widget_form_submit($form, &$form_state) {
  // Emulate the D7 arrays.
  $field = $form_state['values']['workflow_field'];
  $instance = $form_state['values']['workflow_instance'];
  $entity_type = $form['#entity_type'];
  $entity = $form['#entity'];
  $items[0]['workflow'] = $form_state['values'];

  // Submit the data. $items is reset by reference to normal value, and is magically saved by the field itself.
  $widget = new WorkflowDefaultWidget($field, $instance, $entity_type, $entity);
  $widget->submit($form, $form_state, $items); // $items is a proprietary D7 parameter.

  // Remember: we are on a Node view, so the Widget->submit saves the scheduled transition, 
  // but not the new state of the node itself.
  // Widget::submit() returns the new value in a 'sane' state.
  // Save the referenced entity, but only is transition succeeded, and is not scheduled.
  $field_name = $field['field_name'];
  $old_sid = _workflow_get_sid_by_items($entity->{$field_name}['und']);
  $new_sid = _workflow_get_sid_by_items($items);
  if ($old_sid != $new_sid) {
      $entity->{$field_name}['und'] = $items;
      $widget->entitySave($entity_type, $entity);
  }

}