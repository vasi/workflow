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

  if ($entity_type == 'comment' || // No Workflow form on a comment display, (Also, this avoids a lot of error messages.)
      $entity->status == 0 ||      // or if the entity is not published,  
      !user_access('show workflow state form') // or if the user has no access to Workflow Form.
      ) {
    return $list_element;
  }

  $state = WorkflowState::load($current_sid);
  $options = $state->getOptions($entity_type, $entity, $force = FALSE);

  $workflow = $state->getWorkflow();

  // Only build form if user has possible target state(s).
  if (!workflow_show_form($current_sid, $workflow, $options)) {
//dpm('show only formatter', __FUNCTION__);
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
  $entity_id = isset($entity->nid) ? $entity->nid : entity_id($entity_type, $entity);
  $element += drupal_get_form(WORKFLOWFIELD_FORM . '_' . $entity_id, $field, $instance, $langcode, $items, $delta, $element);

  return $element;
}
