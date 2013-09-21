<?php

/**
 * @file
 * Contains workflow\includes\Field\WorkflowItem.
 * @see https://drupal.org/node/2064123 for 'Field Type Plugin' change record D7->D8.
 */

/**
 * Plugin implementation of the 'workflow' field type.
 *
 * @FieldType(
 *   id = "workflow",
 *   label = @Translation("Workflow"),
 *   description = @Translation("This field stores Workflow values for a certain Workflow type from a list of allowed 'value => label' pairs, i.e. 'Publishing': 1 => unpublished, 2 => draft, 3 => published."),
 *   default_widget = "options_select",
 *   default_formatter = "list_formatter"
 * )
 */
class WorkflowItem extends WorkflowD7Base { // D8: extends ConfigFieldItemBase implements PrepareCacheInterface {
  /*
   * Function, that gets replaced by the 'annotations' in D8. (@see comments above this class)
   */
  public static function getInfo() {
    return array(
      'workflow' => array(
        'label' => t('Workflow'),
        'description' => t("This field stores Workflow values for a certain Workflow type from a list of allowed 'value => label' pairs, i.e. 'Publishing': 1 => unpublished, 2 => draft, 3 => published."),
        'settings' => array(
            'allowed_values_function' => 'workflowfield_allowed_values', // For the list.module formatter
     //       'allowed_values_function' => 'WorkflowItem::getAllowedValues', // For the list.module formatter
            'wid' => '',
//            'history' => 1,
//            'schedule' => 0,
//            'comment' => 0,
            'widget' => array(
              'options' => 'select',
              'name_as_title' => 1,
              'schedule' => 1,
              'schedule_timezone' => 1,
              'comment' => 1,
            ),
            'watchdog_log' => 1,
            'history' => array(
              'show' => 1,
              'roles' => array(),
            ),
          ),
        'default_widget' => 'options_select',
        'default_formatter' => 'list_default',
      ),
    );
  }

  /*
   * Implements hook_field_settings_form() -> ConfigFieldItemInterface::settingsForm()
   */
  public function settingsForm(array $form, array &$form_state, $has_data) {
    $field_info = self::getInfo();
    $settings = $this->field['settings'];
    $settings += $field_info['workflow']['settings'];
    $settings['widget'] += $field_info['workflow']['settings']['widget'];

    $wid = $this->field['settings']['wid'];
    // Create list of all Workflow types. Include an initial empty value.
    $workflows = array();
    $workflows[''] = t('- Select a value -');
    foreach (Workflow::getWorkflows() as $workflow) {
      $workflows[$workflow->wid] = $workflow->name;
    }

    // The allowed_values_functions is used in the formatter from list.module.
    $element['allowed_values_function'] = array(
      '#type' => 'value',
      '#value' => $settings['allowed_values_function'], // = 'workflowfield_allowed_values',
    );

    // Let the user choose between the available workflow types.
    $element['wid'] = array(
      '#type' => 'select',
      '#title' => t('Workflow type'),
      '#options' => $workflows,
      '#default_value' => $wid,
      '#required' => TRUE,
      '#disabled' => $has_data,
      '#description' => t('Choose the Workflow type.'),
    );

    // Inform the user of possible states.
    // If no Workflow type is selected yet, do not show anything.
    if ($wid) {
      // Get a string representation to show all options.
      $allowed_values_string = $this->_allowed_values_string($wid);

      $element['allowed_values_string'] = array(
        '#type' => 'textarea',
        '#title' => t('Allowed values for the selected Workflow type'),
        '#default_value' => $allowed_values_string,
        '#rows' => 10,
        '#access' => TRUE, // user can see the data,
        '#disabled' => TRUE, //.. but cannot change them.
      );
    }

    $element['widget'] = array(
      '#type' => 'fieldset',
      '#title' => t('Workflow widget'),
      '#description' => 'Set some global properties of the widgets for this workflow. Some can be altered per widget instance.',
    );
    $element['widget']['options'] = array(
      '#type' => 'select',
      '#title' => t('How to show the available states'),
      '#required' => FALSE,
      '#default_value' => $settings['widget']['options'],
//      '#multiple' => TRUE / FALSE,
      '#options' => array(  // These options are taken from options.module
                          'select' => 'Select list',
                          'radios' => 'Radio buttons',
//                          'actions' => 'Action buttons', // by workflow contrib.
                         ),
      '#description' => t('The Widget shows all available states. Decide which is the best way to show them.'),
    );
    $element['widget']['name_as_title'] = array(
      '#type' => 'checkbox',
      '#attributes' => array('class' => array('container-inline')),
      '#title' => t('Use the workflow name as the title of the workflow form'),
      '#default_value' => $settings['widget']['name_as_title'],
      '#description' => t('The workflow section of the editing form is in its own fieldset. Checking the box will add the workflow ' .
      'name as the title of workflow section of the editing form.'),
    );
    $element['widget']['schedule'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow scheduling of workflow transitions.'),
      '#required' => FALSE,
      '#default_value' => $settings['widget']['schedule'],
      '#description' => t('Workflow transitions may be scheduled to a moment in the future. ' .
        'Soon after the desired moment, the transition is executed by Cron. ' .
        'This may be hidden by settings in widgets, formatters or permissions.'),
    );
    $element['widget']['schedule_timezone'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show a timezone when scheduling a transition.'),
      '#required' => FALSE,
      '#default_value' => $settings['widget']['schedule_timezone'],
    );
    $element['widget']['comment'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow adding a comment to workflow transitions.'),
      '#required' => FALSE,
      '#default_value' => $settings['widget']['comment'],
      '#description' => t('On the Workflow form, a Comment form can be included so that the person making the state change can record ' .
        'reasons for doing so. The comment is then included in the node\'s workflow history. ' .
        'This may be hidden by settings in widgets, formatters or permissions.'),
    );

    $element['watchdog_log'] = array(
      '#type' => 'checkbox',
      '#attributes' => array('class' => array('container-inline')),
      '#title' => t('Log informational watchdog messages when a transition is executed (state of a node is changed)'),
      '#default_value' => $settings['watchdog_log'],
      '#description' => t('Optionally log transition state changes to watchdog.'),
    );

    $element['history'] = array(
      '#type' => 'fieldset',
      '#title' => t('Workflow history'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $element['history']['show'] = array(
      '#type' => 'checkbox',
      '#title' => t('Use the workflow history, and show it on a separate tab.'),
      '#required' => FALSE,
      '#default_value' => $settings['history']['show'],
      '#description' => t('If checked, the state change is recorded in table {workflow_node_history}, ' .
        "and a tab 'Worklow' is shown on the node page, which gives access to the History of the workflow."),
    );
    $element['history']['roles'] = array(
      '#type' => 'checkboxes',
      '#options' => workflow_admin_ui_get_roles(),
      '#title' => t('Workflow history permissions'),
      '#default_value' => explode(',', $workflow->tab_roles),
      '#description' => t('Select any roles that should have access to the workflow tab on nodes that have a workflow.'),
    );

    return $element;
  }

  /*
   * Currently, there are no instance Settings.
   * hook_field_instance_settings_form() -> ConfigFieldItemInterface::instanceSettingsForm()
   */
//  public function instanceSettingsForm(array $form, array &$form_state, $has_data) {
//  }

/**
 * Do not implement hook_field_presave(), 
 * since $nid is needed, but not yet known at this moment.
 * hook_field_presave() -> FieldItemInterface::preSave()
 */
//function workflowfield_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
//}

/**
 * Implements hook_field_insert() -> FieldItemInterface::insert()
 */
  public function insert() {
    return $this->update();
  }

/**
 * Implements hook_field_update() -> FieldItemInterface::update()
 * 
 * It is called also from hook_field_insert(), since we need $nid to store {workflow_node_history}.
 * We cannot use hook_field_presave(), since $nid is not yet known at that moment.
 *
 * "Contrary to the old D7 hooks, the methods do not receive the parent entity 
 * "or the langcode of the field values as parameters. If needed, those can be accessed 
 * "by the getEntity() and getLangcode() methods on the Field and FieldItem classes.
 *
 */
  public function update(&$items) { // ($entity_type, $entity, $field, $instance, $langcode, &$items) {
    $field_name = $this->field['field_name'];
    $wid = $this->field['settings']['wid'];
    $new_sid = _workflow_get_sid_by_items($items);
    $new_state = new WorkflowState($new_sid, $wid);

    // @todo D8: remove below lines.
    $entity = $this->entity;
    $entity_type = $this->entity_type;

    $nid = isset($entity->nid) ? $entity->nid : 0;
    if ($nid && $this->entity_type == 'comment') {
      // This happens when we are on an entity's comment.
      // todo: for now, if nid is set, then it is a node. What happens with other entities?
      $referenced_entity_type = 'node';
      $referenced_entities = entity_load($referenced_entity_type, array($nid));
      $referenced_entity = $referenced_entities[$nid];

      // Submit the data. $items is reset by reference to normal value, and is magically saved by the field itself.
      $form = array();
      $form_state = array();
      $form['#node'] = $referenced_entity;
      $form['#node_type'] = $referenced_entity_type;
      $widget = new WorkflowDefaultWidget($this->field, $this->instance);
      $widget->submit($form, $form_state, $items); // $items is a proprietary D7 parameter.

      // Remember: we are on a comment form, so the comment is saved automatically, the referenced entity not.
      // @todo: probably we'd like to do this form within the Widget, but that does not know
      //        wether we are on a comment or a node form.
      //
      // submit() returns the new value in a 'sane' state.
      // Save the referenced entity, but only is transition succeeded, and is not scheduled. 
      $old_sid = _workflow_get_sid_by_items($referenced_entity->{$field_name}['und']);
      $new_sid = _workflow_get_sid_by_items($items);
      if ($old_sid != $new_sid) {
        $referenced_entity->{$field_name}['und'] = $items;
        node_save($referenced_entity);
      }

    }
    elseif ($nid && $this->entity_type != 'comment') {
      if (isset($items[0]['value'])) {
        // A 'normal' options.module-widget is used, and $items[0]['value'] is already properly set.
      }
      elseif (isset($items[0]['workflow'])) {
        // The WorkflowDefaultWidget is used.
        $form['#node'] = $entity;

        // Submit the data. $items is reset by reference to normal value, and is magically saved by the field itself.
        $form = array();
        $form_state = array();
        $form['#node'] = $entity;
        $form['#node_type'] = $entity_type;
        $widget = new WorkflowDefaultWidget($this->field, $this->instance);
        $widget->submit($form, $form_state, $items); // $items is a proprietary D7 parameter.
      }
      else {
        drupal_set_message('error', 'error');
      }
    }
    elseif (!$nid && $entity_type == 'comment') {
      // not possible: a comment on a non-existent node.
    }
    elseif (!$nid && $entity_type != 'comment') {
      if ($entity) {
        // A 'normal' node add page.
        // We should not be here, since we only do inserts after $nid is known.
        $current_sid = workflow_get_creation_state_by_wid($wid);
        $current_sid = Workflow::getWorkflow($wid)->getCreationState();
      }
      else {
        // No entity available, we are on the field Settings page - 'default value' field.
        // This is hidden from the admin, because the default value can be different for every user.
      }
    }
  } 

/**
 * Implements hook_field_delete() -> FieldItemInterface::delete()
 */
//  public function delete() {
//  }


  public function getCurrentState() {
    $field_name = $this->field['field_name'];
    $wid = $this->field['settings']['wid'];
    $workflow = new Workflow($wid);

    $options = array();

    $entity = $this->entity;
    $entity_type = $this->entity_type;

    $nid = isset($entity->nid) ? $entity->nid : 0;
    if ($nid && $this->entity_type == 'comment') {
      // This happens when we are on an entity's comment.
      // We need to fetch the field value of the original node, and show it on the comment. 

      $entity_type = 'node'; // Comments only exist on nodes.
      $referenced_entities = entity_load($entity_type, array($nid));
      $entity = $referenced_entities[$nid];

      $items = field_get_items($entity_type, $entity, $field_name, $langcode = NULL);
      $state = new WorkflowState(_workflow_get_sid_by_items($items), $wid);
      if (!$state) {
        // E.g., the node was created before the field was added: do the same as 'Node Add' page.
        $state = $workflow->getCreationState();
      }
    }
    elseif ($nid && $this->entity_type != 'comment') {
      // A 'normal' node edit page.
      $items = field_get_items($entity_type, $entity, $field_name, $langcode = NULL);
      $state = new WorkflowState(_workflow_get_sid_by_items($items), $wid);
      if (!$state) {
        // E.g., the node was created before the field was added: do the same as 'Node Add' page.
        $state = $workflow->getCreationState();
      }
    }
    elseif (!$nid && $entity_type == 'comment') {
      // not possible: a comment on a non-existent node.
    }
    elseif (!$nid && $entity_type != 'comment') {
      if ($entity) {
        // A 'normal' node add page.
        $state = $workflow->getCreationState();
      }
      else {
        // No entity available, we are on the field Settings page - 'default value' field.
        // This is hidden from the admin, because the default value can be different for every user.
        $state = NULL;
      }
    }

    return $state;
  }

  /*
   * Helper functions for the Field Settings page.
   * Generates a string representation of an array of 'allowed values'.
   * This is a copy from list.module's list_allowed_values_string().
   *
   * This string format is suitable for edition in a textarea.
   *
   * @param $values
   *   An array of values, where array keys are values and array values are
   *   labels.
   *
   * @return
   *   The string representation of the $values array:
   *    - Values are separated by a carriage return.
   *    - Each value is in the format "value|label" or "value".
   */
  private function _allowed_values_string($wid = 0) {
    $lines = array();
    $states = $wid ? workflow_get_workflow_states_by_wid($wid) : workflow_get_workflow_states();
    $last_wid = -1;
  
    foreach ($states as $state) {
      // Only show enabled states.
      if ($state->status) {
        if (($wid == 0) && ($last_wid <> $state->wid)) {
          // Show a Workflow name between Workflows, if more then 1 in the list.
          $last_wid = $state->wid;
          $lines[] = $state->name . "'s states: ";
        }
        $states[$state->sid] = check_plain(t($state->state));
        $lines[] = $state->sid . ' | ' . check_plain(t($state->state));
      }
    }
    return implode("\n", $lines);
  }

  /*
   * Helper function for list.module formatter.
   *
   * Returns the array of allowed values for a list field.
   * Used as a callback function in the list module.
   * @see list_allowed_values() : 
   * "The strings are not safe for output. Keys and values of the array should be
   * "sanitized through field_filter_xss() before being displayed.
   *
   * @return
   *   The array of allowed values. Keys of the array are the raw stored values
   *   (number or text), values of the array are the display labels.
   *
   * @todo: this function only needs to return the current value, not ALL.
   */
  public function getAllowedValues() {
    $options = array();
    if ($state = $this->getCurrentState()) {
      $options = array($state->sid => $state->getName());
    }
    return $options;
  }

  /**
   * Callback function for the default Options widgets.
   * This function really does a getOptions for a State.
   * However, a State is not aware of the entity (node or comment)
   * So, this Workflow Field function serves as a wrapper. 
   */
  public function getOptions() {
    $state = $this->getCurrentState();
    $options = workflow_field_choices($this->entity, FALSE, $state);

    return $options;
  }

}
