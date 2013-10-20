<?php

/**
 * @file
 * Contains workflow\includes\Field\WorkflowDefaultWidget.
 */

/**
 * Plugin implementation of the 'workflow_default' widget.
 * @todo D8: Replace "extends WorkflowD7WidgetBase" by "extends WidgetBase"
 *           or perhaps by "extends OptionsWidgetBase" from Options module.
 *
 * @FieldWidget(
 *   id = "workflow_default",
 *   label = @Translation("Workflow"),
 *   field_types = {
 *     "workflow"
 *   },
 *   settings = {
 *     "name_as_title" = 1
 *     "comment" = 1
 *   }
 * )
 */
class WorkflowDefaultWidget extends WorkflowD7Base { // D8: extends WidgetBase {
  /*
   * Function, that gets replaced by the 'annotations' in D8. (@see comments above this class)
   */
  public static function settings() {
    return array(
      'workflow_default' => array(
        'label' => t('Workflow'),
        'field types' => array('workflow'),
        'settings' => array(
            'name_as_title' => 1,
            'comment' => 1,
          ),
      ),
    );
  }

  /**
   * Implements hook_field_widget_settings_form() --> WidgetInterface::settingsForm().
   * {@inheritdoc}
   *
   * The Widget Instance has no settings. To have a uniform UX, all settings are done on the Field level.
   */
  public function settingsForm(array $form, array &$form_state, $has_data) {
    $element = array();
    return $element;
  }

  /**
   * Implements hook_field_widget_form --> WidgetInterface::formElement().
   * {@inheritdoc}
   *
   * Be careful: this widget may be shown in very different places. Test carefully!!
   *  - On a node add/edit page
   *  - On a node preview page
   *  - On a node view page
   *  - On a node 'workflow history' tab
   *  - On a comment display, in the comment history
   *  - On a comment form, below the comment history
   * @todo D8: change "array $items" to "FieldInterface $items"
   */
  public function formElement(array $items, $delta, array $element, $langcode, array &$form, array &$form_state) {
    $field_name = $this->field['field_name'];
    $entity = $this->entity;
    $entity_type = $this->entity_type;
    $entity_id = isset($entity->nid) ? $entity->nid : entity_id($entity_type, $entity);

    if (!$entity) {
      // If no entity given, do not show a form. E.g., on the field settings page.
      return $element;
    }

    // Capture settings to format the form/widget.
    $settings_title_as_name = ($this->field['settings']['widget']['name_as_title'] && $this->instance['widget']['settings']['name_as_title']);
    // The schedule cannot be shown on a Content add page.
    $settings_schedule = $this->field['settings']['widget']['schedule'] && $entity_id;
    $settings_schedule_timezone = $this->field['settings']['widget']['schedule_timezone'];
    // Show comment, when Field ánd Instance allow this.
    $settings_comment = ($this->field['settings']['widget']['comment'] && $this->instance['widget']['settings']['comment']) ? 'textarea' : 'hidden';

    // The 'add submit' setting is explicitely set by workflowfield_field_formatter_view(), to add the submit button on the Content view page.
    $settings_submit = isset($this->instance['widget']['settings']['submit']) ? TRUE : FALSE;
    $workflow = Workflow::load($this->field['settings']['wid']);

    // @todo: Get the current sid for content, comment, preview.
    if (count($items)) {
      // A normal Content edit.
      $sid = _workflow_get_sid_by_items($items);
    }
    else {
      // We are on a Content add or Comment add (which do not have a state, yet),
      // or we are viewing existing content, which didn't have a state before.
      //@todo: why/when would field_get_items returns a result, if $items is already empty?
      $items = field_get_items($entity_type, $entity, $field_name);
      if ($items) {
        $sid = _workflow_get_sid_by_items($items);
      }
    }
    if (empty($sid)) {
        // Content add page: No valid sid is given, so get the first state.
        // or a states has been deleted.
        $sid = $workflow->getFirstSid($entity_type, $entity);
      }

    $state = WorkflowState::load($sid);
    $options = $state->getOptions($entity_type, $entity);

    // Get the scheduling info. This may change the current $sid on the Form.
    $scheduled = '0';
    $timestamp = REQUEST_TIME;
    $comment = NULL;

    if ($settings_schedule) {
      // Read scheduled information.
      // Technically you could have more than one scheduled, but this will only add the soonest one.
      foreach (WorkflowScheduledTransition::load($entity_type, $entity_id, $field_name) as $scheduled_transition) {
        $scheduled = '1';
        $sid = $scheduled_transition->sid;
        $timestamp = $scheduled_transition->scheduled;
        $comment = $scheduled_transition->comment;
        break;
      }
    }

    // Stop if user has no new target state(s) to choose.
    if (!workflow_show_form($sid, $workflow, $options)) {
//dpm('show only formatter for ' . $sid , __FUNCTION__);
      return $element;
    }

    $label = $workflow->label();
    $element['workflow'] = array(
      '#type' => 'fieldset',
      '#title' => $label,
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
//      '#weight' => 10,
      );

    // Save the current value of the node in the form, for later reference.
    $element['workflow']['#node'] = $entity;
    $element['workflow']['#entity'] = $entity;
    $element['#node'] = $entity;
    $element['#entity'] = $entity;

//dpm(array_pop($options));
//dpm(array_pop($options));
    if (count($options) == 1) {
    // Add the State widget/formatter
    // @todo: add real formatter, instead.
    // @todo: TEST THIS USE CASE.
      // There is no need to show the single choice.
      // A form choice would be an array with the key of the state.
      $state = key($options);
//      $element['workflow'][$label] = array(
      $element['workflow']['workflow_options'] = array(
        '#type' => 'value',
        '#value' => array($state => $state),
        );
    }
    else {
      // @todo: Q: why are we overwriting 'fieldset' with 'container' ?
      //        A: this is because you'd want the form in a vertical tab, and a fieldset makes no sense there.
      $element['workflow']['#type'] = 'container';
      $element['workflow']['#attributes'] = array('class' => array('workflow-form-container'));


      $element['workflow']['workflow_options'] = array(
        '#type' => $this->field['settings']['widget']['options'],
        '#title' => $settings_title_as_name ? t('Change !name state', array('!name' => $label)) : '',
        '#options' => $options,
//        '#name' => $label,
//        '#parents' => array('workflow'),
        '#default_value' => $sid,
        );
    }

    // Display scheduling form, but only if entity is being edited and user has
    // permission. State change cannot be scheduled at entity creation because
    // that leaves the entity in the (creation) state.
    if ($settings_schedule == TRUE
        // && !(arg(0) == 'node' && arg(1) == 'add') // This is already tackled by checking $entity_id
        && user_access('schedule workflow transitions')) {

      // Caveat: for the #states to work in multi-node view, the name is suffixed by unique ID.
      if (isset($form['#id']) && $form['#id'] == 'comment-form') {
        // This is already the name for Node API and now also for Comment form.
        // We assume there is only one Comment form on a page.
        $element_name = 'workflow_scheduled';
      }
      else {
        // This name allows for multiple nodes on a page.
        // @todo: #states doesn't work yet for non-Node entities.
        $element_name = 'workflow_scheduled' . '-' . $entity_type . '-' . $entity_id;
      }

//    $element['workflow']['workflow_scheduled'] = array(
    $element['workflow'][$element_name] = array(
      '#type' => 'radios',
      '#title' => t('Schedule'),
      '#options' => array(
          '0' => t('Immediately'),
          '1' => t('Schedule for state change'),
        ),
      '#default_value' => $scheduled,
      );
    $element['workflow']['workflow_scheduled_date_time'] = array(
      '#type' => 'fieldset',
      '#title' => t('At'),
      '#prefix' => '<div style="margin-left: 1em;">',
      '#suffix' => '</div>',
      '#states' => array(
//        'visible' => array(':input[name="workflow_scheduled"]' => array('value' => '1')),
//        'invisible' => array(':input[name="workflow_scheduled"]' => array('value' => '0')),
        'visible' => array(':input[name="' . $element_name . '"]' => array('value' => '1')),
        'invisible' => array(':input[name="' . $element_name . '"]' => array('value' => '0')),
        ),
      );
      $element['workflow']['workflow_scheduled_date_time']['workflow_scheduled_date'] = array(
        '#type' => 'date',
        '#default_value' => array(
          'day'   => date('j', $timestamp),
          'month' => date('n', $timestamp),
          'year'  => date('Y', $timestamp),
        ),
      );

      $hours = format_date($timestamp, 'custom', 'H:i');
      $element['workflow']['workflow_scheduled_date_time']['workflow_scheduled_hour'] = array(
        '#type' => 'textfield',
        '#description' => t('Please enter a time in 24 hour (eg. HH:MM) format.
          If no time is included, the default will be midnight on the specified date.
          The current time is: @time', array('@time' => $hours)),
        '#default_value' => $scheduled ?
          (isset($form_state['values']['workflow_scheduled_hour']) ?
            $form_state['values']['workflow_scheduled_hour'] : $hours) : '00:00',
        );

      global $user;
      if (variable_get('configurable_timezones', 1) && $user->uid && drupal_strlen($user->timezone)) {
        $timezone = $user->timezone;
      }
      else {
        $timezone = variable_get('date_default_timezone', 0);
      }

      $timezones = drupal_map_assoc(timezone_identifiers_list());

      $element['workflow']['workflow_scheduled_date_time']['workflow_scheduled_timezone'] = array(
        '#type' => $settings_schedule_timezone ? 'select' : 'hidden',
        '#options' => $timezones,
        '#title' => t('Time zone'),
        '#default_value' => array($timezone => $timezone),
        );
    }

    $element['workflow']['workflow_comment'] = array(
      '#type' => $settings_comment,
      '#title' => t('Workflow comment'),
      '#description' => t('A comment to put in the workflow log.'),
      '#default_value' => $comment,
      '#rows' => 2,
      );

    if ($settings_submit) {
      // Add a submit button, but only on Entity View and History page.
      $element['workflow']['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Update workflow'),
        '#executes_submit_callback' => TRUE,
        '#submit' => array('workflowfield_field_widget_form_submit'),
        );
    }

    return $element;
  }

  /*
   * Implements workflow_transition() -> WorkflowDefaultWidget::submit()
   * Overrides submit(array $form, array &$form_state).
   * Contains 2 extra parameters for D7
   * @param array $items
   *   The value of the field.
   * @param array $force
   *   A boolean. TRUE if all access must be overridden, e.g., for Rules.
   *
   * This is called from function _workflowfield_form_submit($form, &$form_state)
   * It is a replacement of function workflow_transition($node, $new_sid, $force, $field)
   * It performs the following actions;
   * - save a scheduled action
   * - update history
   * - restore the normal $items for the field.
   * @todo: remove update of {node_form} table. (separate task, because it has features, too)
   */
  public function submit(array $form, array &$form_state, array &$items, $force = FALSE) {
    global $user;

    $entity_type = $this->entity_type;
    $entity = $this->entity;
    $entity_id = isset($entity->nid) ? $entity->nid : entity_id($entity_type, $entity);
    $field = $this->field;
    $field_name = isset($this->field['field_name']) ? $this->field['field_name'] : '';

    // Massage the items, depending on the type of widget.
    // @todo: use MassageFormValues($values, $form, $form_state).
    $old_sid = workflow_node_current_state($entity, $entity_type, $field); // Todo : entity support.
    $new_sid = isset($items[0]['workflow']['workflow_options']) ? $items[0]['workflow']['workflow_options'] : $items[0]['value'];
    $new_items = isset($items[0]['workflow']) ? $items[0]['workflow'] : $items;

    $transition = $this->getTransition($old_sid, $new_sid, $new_items);

    if ($error = $transition->isAllowed($force)) {
      drupal_set_message($error, 'error');
    }
    elseif (!$transition->isScheduled()) {
      // Now the data is captured in the Transition, and before calling the Execution,
      // restore the default values for Workflow Field.
      // For instance, workflow_rules evaluates this.
      if ($field_name) {
        $items = array();
        $items[0]['value'] = $new_sid;
        $entity->{$field_name}['und'] = $items;
      }

      // It's an immediate change. Do the transition.
      // - validate option; add hook to let other modules change comment.
      // - add to history; add to watchdog
      // return the new value of the sid. (Execution may fail and return the old Sid.)
      $new_sid = $transition->execute($force);

      // In case the transition is not executed, reset the old value.
      if ($field_name) {
        $items = array();
        $items[0]['value'] = $new_sid;
        $entity->{$field_name}['und'] = $items;
      }
    }
    else {
      // A scheduled transition must only be saved to the database. The entity is not changed.
      $transition->save();

      // The current value is still the previous state.
      $new_sid = $old_sid;
    }

    // The entity is still to be saved, so set to a 'normal' value.
    if ($field_name) {
      $items = array();
      $items[0]['value'] = $new_sid;
      $entity->{$field_name}['und'] = $items;
    }
    return $new_sid;
  }

  /*
   * Implements hook_field_widget_error --> WidgetInterface::errorElement().
   */
  public function errorElement(array $element, ConstraintViolationInterface $violation, array $form, array &$form_state) {
  }

  public function settingsSummary() {
  }

//  public function massageFormValues(array $values, array $form, array &$form_state) {
//  }

  /*
   * Extract a WorkflowTransition or a WorkflowScheduledTransition from the form.
   * @todo: move validation and messages to errorElement();
   */
  function getTransition($old_sid, $new_sid, array $form_data) {
    global $user;

    $entity_type = $this->entity_type;
    $entity = $this->entity;
    $entity_id = isset($entity->nid) ? $entity->nid : entity_id($entity_type, $entity);
    $comment = isset($form_data['workflow_comment']) ? $form_data['workflow_comment'] : '';
    $field_name = !empty($this->field) ? $this->field['field_name'] : '';

    // Caveat: for the #states to work in multi-node view, the name is suffixed by unique ID.
    // We check both variants, for Node API and Field API, for backwards compatibility.
    $element_name = 'workflow_scheduled' . '-' . $entity_type . '-' . $entity_id;
    $scheduled = ( isset($form_data['workflow_scheduled']) ? $form_data['workflow_scheduled'] : 0 ) ||
                 ( isset($form_data[$element_name]) ? $form_data[$element_name] : 0 );
    if (!$scheduled) {
      $stamp = REQUEST_TIME;
      $transition = new WorkflowTransition($entity_type, $entity, $field_name, $old_sid, $new_sid, $user->uid, $stamp, $comment);
    }
    else {
      // Schedule the time to change the state.
      // If $form_data is passed, use plain values; if $form is passed, use fieldset 'workflow_scheduled_date_time'.
      $schedule = isset($form_data['workflow_scheduled_date_time']) ? $form_data['workflow_scheduled_date_time'] : $form_data;
      if (!isset($schedule['workflow_scheduled_hour'])) {
        $schedule['workflow_scheduled_hour'] = '00:00';
      }

      $scheduled_date_time =
          $schedule['workflow_scheduled_date']['year']
        . substr('0' . $schedule['workflow_scheduled_date']['month'], -2, 2)
        . substr('0' . $schedule['workflow_scheduled_date']['day'], -2, 2)
        . ' '
        . $schedule['workflow_scheduled_hour']
        . ' '
        . $schedule['workflow_scheduled_timezone']
        ;

      if ($stamp = strtotime($scheduled_date_time)) {
        $transition = new WorkflowScheduledTransition($this->entity_type, $this->entity, $field_name, $old_sid, $new_sid, $user->uid, $stamp, $comment);
      }
      else {
        $transition = NULL;
      }
    }
    return $transition;
  }

}
